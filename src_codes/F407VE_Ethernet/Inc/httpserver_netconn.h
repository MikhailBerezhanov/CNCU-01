#ifndef __HTTPSERVER_NETCONN_H__
#define __HTTPSERVER_NETCONN_H__

typedef enum{
	unknown = 0,
	start,
	in,
	out,
	finish
} ssi_state;

void http_server_netconn_init(void);
void DynWebPage(struct netconn *conn);
void http_server_serve(struct netconn *conn);
void httpd_SSI_init(void);
#endif /* __HTTPSERVER_NETCONN_H__ */

