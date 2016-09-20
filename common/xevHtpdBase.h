/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/

 
#ifndef _XEVHTTPD_BASE_H_
#define _XEVHTTPD_BASE_H_

#include <event.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <dirent.h>

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>

#include <assert.h>
#include <iostream>
#include <thread>
#include <evhtp.h>


using namespace std;
#define  stricmp strcasecmp
#define  strnicmp strncasecmp
#define  SLEEP sleep
#define  SERVER_SIGNATURE "xhttpcache"


class xevHtpdBase{
public:
    xevHtpdBase();
    ~xevHtpdBase();

public:
    static const char *GuessContentType(const char *path);
    static int dump_header(evhtp_kv_t * kv, void * arg);
    static void HttpDebug(evhtp_request_t  *req, int debug=0);
    static void DebugHttpHeader(evhtp_request_t  *req);
    static void AddHeader(evhtp_request_t  *req, const char* heardkey, const char* heardval);
    static const char *uri_arg_find(evhtp_request_t *req, const char *arg);
    static void HttpParseURL(evhtp_request_t  *req, struct evkeyvalq *evMyheader);
    static evhtp_query_t *GetHttpPostText(evhtp_request_t *req, struct evkeyvalq *evdata);
    static char* GetHttpPostData(evhtp_request_t *req);
    static void SendStatusResphone(evhtp_request_t *req, int errcode, const char*fmt, ...);
    static void SendHttpResphone(evhtp_request_t  *req, const string & strData);
    static void SendHttpResphone(evhtp_request_t *req, const char* data, int size);
public:
    bool InitSSL(const char *pemfile, const char*privfile, int timeout);
    bool StartHttpd();
    void SetRouteTable(evhtp_t * http);
    bool BindSocket(const char *ip, int port);
    bool Run(int nthreads);
    static void *Dispatch(void *arg);
    static void OnTimer();
    event_base *GetEventBase() {
        return base;
    };
    evhtp_t *getvhtp() {
        return http;
    }
   
private:
    struct event_base *base;
    evhtp_t *http;
    evhtp_ssl_cfg_t *sslcfg;
    bool usessl;
};

#endif
