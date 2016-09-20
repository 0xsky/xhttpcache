/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/

 
#ifndef _XEVHTTPD_BACKENT_H_
#define _XEVHTTPD_BACKENT_H_

#include "xevHtpdBase.h"
#include "xDataBase.h"


class xevHtpdBackend:public xevHtpdBase{
public:
    xevHtpdBackend();
    ~xevHtpdBackend();

public:
    bool Init(xDataBase *db);
    bool InitSysDB();
    bool Start(const char *ip, int port, int threads);

public:
    static void StatisticsCallback(evhtp_request_t  *req, void *arg);
    static void ConfigCallback(evhtp_request_t  *req, void *arg);
    static void RESTCallback(evhtp_request_t  *req, void *arg);
    static void KeyScanCallback(evhtp_request_t  *req, void *arg);
    static void AdminShellCallback(evhtp_request_t  *req, void *arg);
    static void UploadCallback(evhtp_request_t  *req, void *arg);

private:
    static bool ParasrMultipartPost(evhtp_request_t *req, const char* buf, int length, std::string &filedata);
    static void RESTfulPro(evhtp_request_t  *req, xDataBase *xdb);
    static bool CheckAuth(evhtp_request_t  *req);
    static bool CheckSession(evhtp_request_t *req);
public:
    bool SetRouteTable(evhtp_t * http);
    
private:
    xDataBase *xdb;
    xDataBase *sysdb;
};

#endif
