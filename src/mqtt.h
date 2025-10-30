


// 订阅列表数据结构
struct sub {
  struct sub *next;
  struct mg_connection *c;
  struct mg_str topic;
  uint8_t qos;
};
static struct sub *s_subs = NULL;


static size_t mg_mqtt_next_topic(struct mg_mqtt_message *msg,
                                 struct mg_str *topic, uint8_t *qos,
                                 size_t pos) {
  unsigned char *buf = (unsigned char *) msg->dgram.buf + pos;
  size_t new_pos;
  if (pos >= msg->dgram.len) return 0;

  topic->len = (size_t) (((unsigned) buf[0]) << 8 | buf[1]);
  topic->buf = (char *) buf + 2;
  new_pos = pos + 2 + topic->len + (qos == NULL ? 0 : 1);
  if ((size_t) new_pos > msg->dgram.len) return 0;
  if (qos != NULL) *qos = buf[2 + topic->len];
  return new_pos;
}

size_t mg_mqtt_next_sub(struct mg_mqtt_message *msg, struct mg_str *topic,
                        uint8_t *qos, size_t pos) {
  uint8_t tmp;
  return mg_mqtt_next_topic(msg, topic, qos == NULL ? &tmp : qos, pos);
}

size_t mg_mqtt_next_unsub(struct mg_mqtt_message *msg, struct mg_str *topic,
                          size_t pos) {
  return mg_mqtt_next_topic(msg, topic, NULL, pos);
}





// MQTT 协议处理
static void ProcMQTT(struct mg_connection *c, int ev, void *ev_data) {
	XS_ServerObject objServer = (XS_ServerObject)c->fn_data;
	// 先尝试调用自定义的协议处理逻辑
	if ( objServer->DefaultHost.DevLang == SLT_C ) {
		if ( objServer->DefaultHost.EventProc ) {
			int (*EventProc)(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection *c, int ev, void *ev_data) = objServer->DefaultHost.EventProc;
			if ( EventProc(objServer, &objServer->DefaultHost, c, ev, ev_data) ) {
				return;
			}
		}
	} else if ( objServer->DefaultHost.DevLang == SLT_LUA ) {
	} else if ( objServer->DefaultHost.DevLang == SLT_JS ) {
	}
	// 进入协议处理逻辑
	if ( ev == MG_EV_MQTT_CMD ) {
		struct mg_mqtt_message* mm = (struct mg_mqtt_message*)ev_data;
		if ( mm->cmd == MQTT_CMD_CONNECT ) {
			// 客户端连接
			if (mm->dgram.len < 9) {
				mg_error(c, "Malformed MQTT frame");
			} else if (mm->dgram.buf[8] != 4) {
				mg_error(c, "Unsupported MQTT version %d", mm->dgram.buf[8]);
			} else {
				uint8_t response[] = {0, 0};
				mg_mqtt_send_header(c, MQTT_CMD_CONNACK, 0, sizeof(response));
				mg_send(c, response, sizeof(response));
			}
		} else if ( mm->cmd == MQTT_CMD_SUBSCRIBE ) {
			// 订阅消息
			size_t pos = 4;
			uint8_t qos, resp[256];
			struct mg_str topic;
			int num_topics = 0;
			while ( (pos = mg_mqtt_next_sub(mm, &topic, &qos, pos)) > 0) {
				struct sub *sub = calloc(1, sizeof(*sub));
				sub->c = c;
				sub->topic = mg_strdup(topic);
				sub->qos = qos;
				LIST_ADD_HEAD(struct sub, &s_subs, sub);
				MG_INFO( ("SUB %p [%.*s]", c->fd, (int) sub->topic.len, sub->topic.buf) );
				// Change '+' to '*' for topic matching using mg_match
				for (size_t i = 0; i < sub->topic.len; i++) {
					if (sub->topic.buf[i] == '+') ((char *) sub->topic.buf)[i] = '*';
				}
				resp[num_topics++] = qos;
			}
			mg_mqtt_send_header(c, MQTT_CMD_SUBACK, 0, num_topics + 2);
			uint16_t id = mg_htons(mm->id);
			mg_send(c, &id, 2);
			mg_send(c, resp, num_topics);
		} else if ( mm->cmd == MQTT_CMD_PUBLISH ) {
			// 发布消息
			for ( struct sub *sub = s_subs; sub != NULL; sub = sub->next ) {
				if (mg_match(mm->topic, sub->topic, NULL)) {
					struct mg_mqtt_opts pub_opts;
					memset(&pub_opts, 0, sizeof(pub_opts));
					pub_opts.topic = mm->topic;
					pub_opts.message = mm->data;
					pub_opts.qos = 1;
					pub_opts.retain = false;
					mg_mqtt_pub(sub->c, &pub_opts);
				}
			}
		} else if ( mm->cmd == MQTT_CMD_PINGREQ ) {
			// Ping
			mg_mqtt_send_header(c, MQTT_CMD_PINGRESP, 0, 0);
		}
	} else if ( ev == MG_EV_CLOSE ) {
	}
}



// MQTTS 协议处理
static void ProcMQTTS(struct mg_connection *c, int ev, void *ev_data) {
	XS_ServerObject objServer = (XS_ServerObject)c->fn_data;
}





// 启动 MQTT 服务
int RunServerMQTT(XS_ServerObject objServer)
{
	// 启动 MQTT 服务
	printf("\n    Run Server [MQTT] : %s (%s)\n", objServer->Name, objServer->Addr);
	struct mg_connection* objConn = mg_mqtt_listen(&mgr, objServer->Addr, ProcMQTT, objServer);
	if ( objConn == NULL ) {
		printf("    !!! ERROR !!! Cannot listen on %s. Use mqtt://ADDR:PORT or :PORT\n", objServer->Addr);
		exit(EXIT_FAILURE);
	} else {
		objServer->Conn = objConn;
	}
	if ( objServer->EnableTLS ) {
		printf("    Run Server [MQTTS] : %s (%s)\n", objServer->Name, objServer->Addr);
		objConn = mg_mqtt_listen(&mgr, objServer->AddrTLS, ProcMQTTS, objServer);
		if ( objConn == NULL ) {
			printf("    !!! ERROR !!! Cannot listen on %s. Use mqtts://ADDR:PORT or :PORT\n", objServer->AddrTLS);
			exit(EXIT_FAILURE);
		} else {
			objServer->ConnTLS = objConn;
		}
	}
	return TRUE;
}



// 停止 HTTP 服务
int StopServerMQTT(XS_ServerObject objServer)
{
	return TRUE;
}


