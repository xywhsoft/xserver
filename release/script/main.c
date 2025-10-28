


// XS 基础服务库
#include <xsbase.h>



// 导入库
#include "xdo/xdo.h"
#include "xdo/sqlite.h"
//#include "xdo/mysql.h"
//#include "xdo/odbc.h"				/* 可以根据实际使用情况决定是否引用 ODBC 数据库驱动程序 */





// 全局路径
char* ExePath;
char* AppPath;
char* DBPath;
char* TempPath;
char* ToolPath;
char* OptionPath;
char* TemplatePath;



// 全局数据库对象
XDO_Connect G_DB;




// 模板相关功能
#include "module/template.h"



// 路由调用 - HTTP
#include "route_http/curd.h"
#include "route_http/chart.h"
#include "route_http/test.h"



// 全局静态路由表
#include "route.h"



// HTTP 协议处理
#include "module/http.h"





#define BACKEND_ADDR "154.64.230.229:22"  // 后端服务地址（如 VNC）

struct backend_conn {
    struct mg_connection *be_conn;   // 到后端的 TCP 连接
    struct mg_connection *ws_conn;   // 关联的 WebSocket 连接
};



// === 后端 TCP 连接处理器 ===
static void backend_handler(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    struct backend_conn *bc = (struct backend_conn *) c->fn_data;

    if (ev == MG_EV_CONNECT) {
		printf("MG_EV_CONNECT\n");
        if (c->is_closing) {
            printf("Failed to connect to backend : 134\n");
            if (bc && bc->ws_conn) {
                mg_ws_send(bc->ws_conn, "Connect error", 13, WEBSOCKET_OP_TEXT);
                mg_close_conn(bc->ws_conn);
            }
            mg_close_conn(c);
        }
    } else if (ev == MG_EV_READ) {
		printf("MG_EV_READ\n");
        // 从后端收到数据，转发给 WebSocket 客户端
        if (bc && bc->ws_conn) {
			printf("TCP Proxy : %.*s\n", c->recv.len, c->recv.buf);
            mg_ws_send(bc->ws_conn, c->recv.buf, c->recv.len, WEBSOCKET_OP_BINARY);
        }
        mg_iobuf_del(&c->recv, 0, c->recv.len);  // 清空接收缓冲区
    } else if (ev == MG_EV_CLOSE) {
		printf("MG_EV_CLOSE\n");
        // 后端断开连接
        if (bc && bc->ws_conn) {
            mg_ws_send(bc->ws_conn, "Backend closed", 14, WEBSOCKET_OP_CLOSE);
            mg_close_conn(bc->ws_conn);  // 关闭前端
        }
        if (bc) bc->be_conn = NULL;
        printf("Backend connection closed\n");
    }
}



// === WebSocket 事件处理器 ===
static void ws_handler(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    struct backend_conn *bc = (struct backend_conn *) c->fn_data;
	
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

		// 升级为 WebSocket
		printf("Upgrading to WebSocket from %.*s\n",
			   (int) hm->uri.len, hm->uri.buf);
		mg_ws_upgrade(c, hm, NULL);
    }
    if (ev == MG_EV_OPEN) {
        // 新 WebSocket 连接建立
        printf("New WebSocket client connected: %d\n", c->id);
    } else if (ev == MG_EV_WS_OPEN) {
		printf("MG_EV_WS_OPEN\n");
        // WebSocket 握手完成，连接就绪
        printf("WebSocket opened for client %d\n", c->id);

        // 创建与后端的连接
        struct mg_connection *be = mg_connect(c->mgr, BACKEND_ADDR, backend_handler, NULL);
        if (be == NULL) {
            printf("Failed to connect to backend %s\n", BACKEND_ADDR);
            mg_ws_send(c, "Backend connection failed", 25, WEBSOCKET_OP_CLOSE);
            return;
        }

        // 分配并绑定上下文
        bc = (struct backend_conn *) calloc(1, sizeof(*bc));
        bc->be_conn = be;
        bc->ws_conn = c;
        c->fn_data = bc;
        be->fn_data = bc;

        printf("Connected to backend: %s\n", BACKEND_ADDR);
    } else if (ev == MG_EV_WS_MSG) {
		printf("MG_EV_WS_MSG\n");
        // 收到 WebSocket 消息
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
		
        if (bc && bc->be_conn) {
            // 转发 WebSocket 数据到后端 TCP
            mg_send(bc->be_conn, wm->data.buf, wm->data.len);
        }
		
        // 处理关闭帧
        if (wm->flags & WEBSOCKET_OP_CLOSE && (wm->data.buf[0] == 0x8 || wm->data.buf[0] == 0x9)) {
            printf("WebSocket control frame (close/ping): %d\n", wm->data.buf[0]);
            if (wm->data.buf[0] == 0x8) {
                mg_close_conn(c);  // Close on close frame
            }
        }
    } else if (ev == MG_EV_CLOSE) {
		printf("MG_EV_CLOSE\n");
        // WebSocket 断开
        if (bc) {
            if (bc->be_conn) {
                bc->be_conn->fn_data = NULL;  // 解绑
                mg_close_conn(bc->be_conn);
            }
            free(bc);
            c->fn_data = NULL;
        }
        printf("WebSocket client disconnected\n");
    }
}





// 服务初始化
void OnError(str sError)
{
	printf("X Runtime Error : %s\n", sError);
}
void ServiceInit(XS_ServerObject objServer)
{
	// 将错误输出到控制台
	xCore->OnError = OnError;
	
	
	
	// 初始化全局目录
	ExePath = xCore->AppPath;
	AppPath = objServer->DefaultHost.Path;
	DBPath = xrtPathJoin(3, ExePath, "data", "db");
	TempPath = xrtPathJoin(3, ExePath, "data", "temp");
	ToolPath = xrtPathJoin(3, ExePath, "data", "tools");
	OptionPath = xrtPathJoin(3, ExePath, "data", "options");
	TemplatePath = xrtPathJoin(3, ExePath, "data", "template");
	
	
	
	// 自动创建目录
	xrtDirCreate(TempPath);
	
	
	
	// 连接到主数据库
	str FileDB = xrtPathJoin(2, DBPath, "main.db");
	G_DB = xdoConnectSQLite(FileDB);
	if ( G_DB == NULL ) {
		printf("!!! ERROR !!! ServiceInit - xdoConnectSQLite error.\n");
	}
	
	
	
	// 初始化模板渲染功能
	InitTemplate();
	
	
	
	// 初始化 HTTP 路由表
	InitRouteHTTP();
	
    /*
    // 绑定监听地址，处理 HTTP 和 WebSocket
    if (!mg_http_listen(mgr, "http://0.0.0.0:8000", ws_handler, NULL)) {
        printf("Failed to bind to http://0.0.0.0:8000\n");
        return 1;
    }

    printf("WebSocket proxy listening on http://0.0.0.0:8000\n");
    printf("Forwarding to backend: %s\n", BACKEND_ADDR);
	*/
	
}



// 服务卸载
void ServiceUnit(XS_ServerObject objServer)
{
	
	// 释放全局路由表
	xrtDictDestroy(StaticRouteTableHTTP);
	
	// 释放数据库
	xdoDisconnect(G_DB);
	sqlite3_shutdown();
	
}