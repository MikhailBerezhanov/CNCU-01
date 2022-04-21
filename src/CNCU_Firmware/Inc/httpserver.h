#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"

#define OLD_HTTP 0

void http_server_serve(struct netconn *conn);

#endif /* __HTTPSERVER_H__ */

