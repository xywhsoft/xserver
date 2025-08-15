void ImportMongoose(TCCState* s)
{
	// 添加函数 - mongoose - FileSystem
	tcc_add_symbol(s, "mg_fs_open", mg_fs_open);
	tcc_add_symbol(s, "mg_fs_close", mg_fs_close);
	tcc_add_symbol(s, "mg_fs_ls", mg_fs_ls);
	tcc_add_symbol(s, "mg_file_read", mg_file_read);
	tcc_add_symbol(s, "mg_file_write", mg_file_write);
	tcc_add_symbol(s, "mg_file_printf", mg_file_printf);
	
	// 添加函数 - mongoose - URL
	tcc_add_symbol(s, "mg_url_port", mg_url_port);
	tcc_add_symbol(s, "mg_url_is_ssl", mg_url_is_ssl);
	tcc_add_symbol(s, "mg_url_host", mg_url_host);
	tcc_add_symbol(s, "mg_url_user", mg_url_user);
	tcc_add_symbol(s, "mg_url_pass", mg_url_pass);
	tcc_add_symbol(s, "mg_url_uri", mg_url_uri);
	
	// 添加函数 - mongoose - Utility
	tcc_add_symbol(s, "mg_call", mg_call);
	tcc_add_symbol(s, "mg_error", mg_error);
	tcc_add_symbol(s, "mg_base64_update", mg_base64_update);
	tcc_add_symbol(s, "mg_base64_final", mg_base64_final);
	tcc_add_symbol(s, "mg_base64_encode", mg_base64_encode);
	tcc_add_symbol(s, "mg_base64_decode", mg_base64_decode);
	tcc_add_symbol(s, "mg_md5_init", mg_md5_init);
	tcc_add_symbol(s, "mg_md5_update", mg_md5_update);
	tcc_add_symbol(s, "mg_md5_final", mg_md5_final);
	tcc_add_symbol(s, "mg_sha1_init", mg_sha1_init);
	tcc_add_symbol(s, "mg_sha1_update", mg_sha1_update);
	tcc_add_symbol(s, "mg_sha1_final", mg_sha1_final);
	tcc_add_symbol(s, "mg_sha256_init", mg_sha256_init);
	tcc_add_symbol(s, "mg_sha256_update", mg_sha256_update);
	tcc_add_symbol(s, "mg_sha256_final", mg_sha256_final);
	tcc_add_symbol(s, "mg_sha256", mg_sha256);
	tcc_add_symbol(s, "mg_hmac_sha256", mg_hmac_sha256);
	tcc_add_symbol(s, "mg_sha384_init", mg_sha384_init);
	tcc_add_symbol(s, "mg_sha384_update", mg_sha384_update);
	tcc_add_symbol(s, "mg_sha384_final", mg_sha384_final);
	tcc_add_symbol(s, "mg_sha384", mg_sha384);
	tcc_add_symbol(s, "mg_random", mg_random);
	tcc_add_symbol(s, "mg_random_str", mg_random_str);
	tcc_add_symbol(s, "mg_crc32", mg_crc32);
	tcc_add_symbol(s, "mg_ntohs", mg_ntohs);
	tcc_add_symbol(s, "mg_ntohl", mg_ntohl);
	tcc_add_symbol(s, "mg_check_ip_acl", mg_check_ip_acl);
	tcc_add_symbol(s, "mg_url_decode", mg_url_decode);
	tcc_add_symbol(s, "mg_url_encode", mg_url_encode);
	
	// 添加函数 - mongoose - String
	tcc_add_symbol(s, "mg_str_s", mg_str_s);
	tcc_add_symbol(s, "mg_str_n", mg_str_n);
	tcc_add_symbol(s, "mg_casecmp", mg_casecmp);
	tcc_add_symbol(s, "mg_strcmp", mg_strcmp);
	tcc_add_symbol(s, "mg_strcasecmp", mg_strcasecmp);
	tcc_add_symbol(s, "mg_strdup", mg_strdup);
	tcc_add_symbol(s, "mg_match", mg_match);
	tcc_add_symbol(s, "mg_span", mg_span);
	tcc_add_symbol(s, "mg_str_to_num", mg_str_to_num);
	tcc_add_symbol(s, "mg_path_is_sane", mg_path_is_sane);
	tcc_add_symbol(s, "mg_aton", mg_aton);
	tcc_add_symbol(s, "mg_vxprintf", mg_vxprintf);
	tcc_add_symbol(s, "mg_xprintf", mg_xprintf);
	tcc_add_symbol(s, "mg_vsnprintf", mg_vsnprintf);
	tcc_add_symbol(s, "mg_snprintf", mg_snprintf);
	tcc_add_symbol(s, "mg_vmprintf", mg_vmprintf);
	tcc_add_symbol(s, "mg_mprintf", mg_mprintf);
	tcc_add_symbol(s, "mg_print_base64", mg_print_base64);
	tcc_add_symbol(s, "mg_print_esc", mg_print_esc);
	tcc_add_symbol(s, "mg_print_hex", mg_print_hex);
	tcc_add_symbol(s, "mg_print_ip", mg_print_ip);
	tcc_add_symbol(s, "mg_print_ip_port", mg_print_ip_port);
	tcc_add_symbol(s, "mg_print_ip4", mg_print_ip4);
	tcc_add_symbol(s, "mg_print_ip6", mg_print_ip6);
	tcc_add_symbol(s, "mg_print_mac", mg_print_mac);
	
	// 添加函数 - mongoose - Time & Timer
	tcc_add_symbol(s, "mg_timer_add", mg_timer_add);
	tcc_add_symbol(s, "mg_timer_init", mg_timer_init);
	tcc_add_symbol(s, "mg_timer_free", mg_timer_free);
	tcc_add_symbol(s, "mg_timer_poll", mg_timer_poll);
	tcc_add_symbol(s, "mg_millis", mg_millis);
	tcc_add_symbol(s, "mg_now", mg_now);
	
	// 添加函数 - mongoose - MQTT
	tcc_add_symbol(s, "mg_mqtt_connect", mg_mqtt_connect);
	tcc_add_symbol(s, "mg_mqtt_listen", mg_mqtt_listen);
	tcc_add_symbol(s, "mg_mqtt_login", mg_mqtt_login);
	tcc_add_symbol(s, "mg_mqtt_pub", mg_mqtt_pub);
	tcc_add_symbol(s, "mg_mqtt_sub", mg_mqtt_sub);
	tcc_add_symbol(s, "mg_mqtt_parse", mg_mqtt_parse);
	tcc_add_symbol(s, "mg_mqtt_send_header", mg_mqtt_send_header);
	tcc_add_symbol(s, "mg_mqtt_ping", mg_mqtt_ping);
	tcc_add_symbol(s, "mg_mqtt_pong", mg_mqtt_pong);
	tcc_add_symbol(s, "mg_mqtt_disconnect", mg_mqtt_disconnect);
	tcc_add_symbol(s, "mg_mqtt_next_prop", mg_mqtt_next_prop);
	
	// 添加函数 - mongoose - SNTP
	tcc_add_symbol(s, "mg_sntp_connect", mg_sntp_connect);
	tcc_add_symbol(s, "mg_sntp_request", mg_sntp_request);
	tcc_add_symbol(s, "mg_sntp_parse", mg_sntp_parse);
	
	// 添加函数 - mongoose - WebSocket
	tcc_add_symbol(s, "mg_ws_connect", mg_ws_connect);
	tcc_add_symbol(s, "mg_ws_upgrade", mg_ws_upgrade);
	tcc_add_symbol(s, "mg_ws_send", mg_ws_send);
	tcc_add_symbol(s, "mg_ws_wrap", mg_ws_wrap);
	tcc_add_symbol(s, "mg_ws_printf", mg_ws_printf);
	tcc_add_symbol(s, "mg_ws_vprintf", mg_ws_vprintf);
	
	// 添加函数 - mongoose - HTTP
	tcc_add_symbol(s, "mg_http_parse", mg_http_parse);
	tcc_add_symbol(s, "mg_http_get_request_len", mg_http_get_request_len);
	tcc_add_symbol(s, "mg_http_printf_chunk", mg_http_printf_chunk);
	tcc_add_symbol(s, "mg_http_write_chunk", mg_http_write_chunk);
	tcc_add_symbol(s, "mg_http_listen", mg_http_listen);
	tcc_add_symbol(s, "mg_http_connect", mg_http_connect);
	tcc_add_symbol(s, "mg_http_serve_dir", mg_http_serve_dir);
	tcc_add_symbol(s, "mg_http_serve_file", mg_http_serve_file);
	tcc_add_symbol(s, "mg_http_reply", mg_http_reply);
	tcc_add_symbol(s, "mg_http_get_header", mg_http_get_header);
	tcc_add_symbol(s, "mg_http_var", mg_http_var);
	tcc_add_symbol(s, "mg_http_get_var", mg_http_get_var);
	tcc_add_symbol(s, "mg_http_creds", mg_http_creds);
	tcc_add_symbol(s, "mg_http_upload", mg_http_upload);
	tcc_add_symbol(s, "mg_http_bauth", mg_http_bauth);
	tcc_add_symbol(s, "mg_http_get_header_var", mg_http_get_header_var);
	tcc_add_symbol(s, "mg_http_next_multipart", mg_http_next_multipart);
	tcc_add_symbol(s, "mg_http_status", mg_http_status);
	tcc_add_symbol(s, "mg_http_serve_ssi", mg_http_serve_ssi);
	
	// 添加函数 - mongoose - Core
	tcc_add_symbol(s, "mg_mgr_poll", mg_mgr_poll);
	tcc_add_symbol(s, "mg_mgr_init", mg_mgr_init);
	tcc_add_symbol(s, "mg_mgr_free", mg_mgr_free);
	tcc_add_symbol(s, "mg_listen", mg_listen);
	tcc_add_symbol(s, "mg_connect", mg_connect);
	tcc_add_symbol(s, "mg_wrapfd", mg_wrapfd);
	tcc_add_symbol(s, "mg_connect_resolved", mg_connect_resolved);
	tcc_add_symbol(s, "mg_send", mg_send);
	tcc_add_symbol(s, "mg_alloc_conn", mg_alloc_conn);
	tcc_add_symbol(s, "mg_close_conn", mg_close_conn);
	tcc_add_symbol(s, "mg_open_listener", mg_open_listener);
	tcc_add_symbol(s, "mg_wakeup", mg_wakeup);
	tcc_add_symbol(s, "mg_wakeup_init", mg_wakeup_init);
	tcc_add_symbol(s, "mg_hello", mg_hello);
	tcc_add_symbol(s, "mg_printf", mg_printf);
	tcc_add_symbol(s, "mg_vprintf", mg_vprintf);
	
	// 添加函数 - mongoose - I/O Buffers
	tcc_add_symbol(s, "mg_iobuf_init", mg_iobuf_init);
	tcc_add_symbol(s, "mg_iobuf_resize", mg_iobuf_resize);
	tcc_add_symbol(s, "mg_iobuf_free", mg_iobuf_free);
	tcc_add_symbol(s, "mg_iobuf_add", mg_iobuf_add);
	tcc_add_symbol(s, "mg_iobuf_del", mg_iobuf_del);
	
	// 添加函数 - mongoose - TLS 1.3
	#if MG_TLS != MG_TLS_MBED
		tcc_add_symbol(s, "mg_tls_init_accept", mg_tls_init_accept);
	#endif
	tcc_add_symbol(s, "mg_tls_init", mg_tls_init);
	tcc_add_symbol(s, "mg_tls_free", mg_tls_free);
	tcc_add_symbol(s, "mg_tls_send", mg_tls_send);
	tcc_add_symbol(s, "mg_tls_recv", mg_tls_recv);
	tcc_add_symbol(s, "mg_tls_pending", mg_tls_pending);
	tcc_add_symbol(s, "mg_tls_handshake", mg_tls_handshake);
	tcc_add_symbol(s, "mg_tls_ctx_init", mg_tls_ctx_init);
	tcc_add_symbol(s, "mg_tls_ctx_free", mg_tls_ctx_free);
	tcc_add_symbol(s, "mg_io_send", mg_io_send);
	tcc_add_symbol(s, "mg_io_recv", mg_io_recv);
}