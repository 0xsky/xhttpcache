/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/

 
#ifndef _XEVHTTPD_FRONTEND_H_
#define _XEVHTTPD_FRONTEND_H_

#include "xevHtpdBase.h"
#include "xDataBase.h"

class xevHtpdFrontend:public xevHtpdBase
{
public:
    xevHtpdFrontend();
    ~xevHtpdFrontend();

public:
    bool Init(xDataBase *db);
    bool Start(const char *ip, int port, int threads);

public:
    bool SetRouteTable(evhtp_t * http);
    static void IndexCallback(evhtp_request_t *req, void *arg);
    static void GetCallback(evhtp_request_t  *req, void *arg);
    static void GetCallbackPro(xevHtpdFrontend *pHttpd, evhtp_request_t *req, const char *uri);
private:
    xDataBase *xdb;
};

#endif
