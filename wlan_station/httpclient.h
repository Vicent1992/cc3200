#ifndef __HTTPCLIENT_H__
#define __HTTPCLIENT_H__

// HTTP Client lib
#include <http/client/httpcli.h>
#include <http/client/common.h>

extern int HTTPGetMethod(HTTPCli_Handle httpClient);
extern int ConnectToHTTPServer(HTTPCli_Handle httpClient);
extern void DisConnectToHTTPServer(HTTPCli_Handle httpClient);

#endif
