/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/

 

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include "xLog.h"
#include "json.h"
#include "xConfig.h"
#include "xevHtpdFrontend.h"
#include "xTimerCount.h"

xevHtpdFrontend::xevHtpdFrontend()
{
    xdb = NULL;
}

xevHtpdFrontend::~xevHtpdFrontend()
{
}

bool xevHtpdFrontend::Init(xDataBase *db) {
    if (db) {
        xdb = db;
        return true;
    }
    return false;
}

bool xevHtpdFrontend::Start(const char *ip, int port, int threads)
{
    if (!BindSocket(ip, port)) {
        log_error("BindSocket error");
        return false;
    }
    if (!SetRouteTable(getvhtp())) {
        log_error("SetRouteTable error");
        return false;
    }
    if (!Run(threads)) {
        log_error("Run error");
        return false;
    }
    return true;
}

bool xevHtpdFrontend::SetRouteTable(evhtp_t * http)
{
    log_info("xevHtpdFrontend SetRouteTable\n");
    struct timeval timeo;
    timeo.tv_sec = xConfig::GetInstance()->frontend.timeout;
    timeo.tv_usec = 0; 
    evhtp_set_timeouts(http, &timeo, &timeo);  

    if (xConfig::GetInstance()->frontend.default_index.length()>0) {
        vector<string> vDest;
        str2Vect(xConfig::GetInstance()->frontend.default_index.c_str(), vDest, "|");
        for (size_t i = 0; i < vDest.size(); ++i) {
            log_debug("IndexCallback: %s", vDest[i].c_str());
            if (!evhtp_set_cb(http, vDest[i].c_str(), xevHtpdFrontend::IndexCallback, this))    return false;
        }
    }

    if (!evhtp_set_glob_cb(http, "/*", xevHtpdFrontend::GetCallback, this))    return false;

    return true;
}

void xevHtpdFrontend::IndexCallback(evhtp_request_t *req, void *arg)
{
    xevHtpdFrontend *pHttpd = reinterpret_cast<xevHtpdFrontend *>(arg);
    //TIME_TEST;
    xevHtpdBase::HttpDebug(req, xConfig::GetInstance()->app.debug);

    GetCallbackPro(pHttpd, req, "index.html");
}

void xevHtpdFrontend::GetCallback(evhtp_request_t *req, void *arg)
{
    xevHtpdFrontend *pHttpd = reinterpret_cast<xevHtpdFrontend *>(arg);
    const char *uri = (char*)req->uri->path->full+1;
    //TIME_TEST;
    xevHtpdBase::HttpDebug(req, xConfig::GetInstance()->app.debug);

    GetCallbackPro(pHttpd, req, uri);
}

void xevHtpdFrontend::GetCallbackPro(xevHtpdFrontend *pHttpd, evhtp_request_t *req, const char *uri)
{
    if ((NULL == uri) || (req->method != htp_method_GET)) {
        SendStatusResphone(req, EVHTP_RES_BADREQ, "error");
        return;
    }

    bool etag_enable = (1 == xConfig::GetInstance()->frontend.etag);
    const char* If_Modified_Since = evhtp_header_find(req->headers_in, "If-Modified-Since");
    const char* If_None_Match = evhtp_header_find(req->headers_in, "If-None-Match");
    
    if ((etag_enable)&&(NULL != If_None_Match)) {
        string str_If_None_Match;
        str_If_None_Match = If_None_Match;
        str_If_None_Match.erase(0, 1);
        str_If_None_Match.erase(str_If_None_Match.end()-1);
        if (pHttpd->xdb->CheckEtag(uri, str_to_int64(str_If_None_Match))) {
            evhtp_send_reply(req, EVHTP_RES_NOTMOD);
            return;
        }
    }

    std::string key(uri);
    xValue value;
    int ret = pHttpd->xdb->Get(key, value);
    if (ret==db_null) {
        SendStatusResphone(req, EVHTP_RES_NOTFOUND, "NOT FOUND");
        return;
    }

    bool Modified = false;
    bool etag     = false;
    if (NULL != If_Modified_Since) {
        char LastModified[32] = { 0 };
        sprintf(LastModified, "%s GMT", toGMT((time_t)value.timestamp / 1000));
        if (0 == strnicmp(If_Modified_Since, LastModified, strlen(LastModified))) {
            Modified = true;
        }
        AddHeader(req, "Last-Modified", LastModified);
    }

    if ((etag_enable)&&(NULL != If_None_Match) ) {
        char Etag[22] = { 0 };
        snprintf(Etag, sizeof(Etag), "\"%" PRId64 "\"", value.timestamp);
        if (0 == strnicmp(Etag, If_None_Match, strlen(Etag))) {
            etag = true;
        }
        AddHeader(req, "ETag", Etag);
    }
    //log_debug("GetCallbackPro evhtp_header etag:%s LastModified:%s ", Etag, LastModified);

    if (Modified || etag) {
        evhtp_send_reply(req, EVHTP_RES_NOTMOD);
        return;
    } 

    const char *type = GuessContentType(uri);
    AddHeader(req, "Content-Type", type);
    if (1 == xConfig::GetInstance()->frontend.gzip) {
        unsigned char* buf = (unsigned char*)malloc(value.strValue.length());
        unsigned long deslen = value.strValue.length();
        int ret = gzcompress((unsigned char*)value.strValue.c_str(), value.strValue.length(), buf, &deslen);
        //log_debug("gzcompress ret:%d length:%d deslen:%d ", ret, value.strValue.length(), deslen);
        if (ret != 0) {
            log_error("gzcompress ret:%d", ret);
            SendHttpResphone(req, value.strValue);
        } else {
            AddHeader(req, "Content-Encoding", "gzip");
            SendHttpResphone(req, (const char*)buf, (int)deslen);
        }
        free(buf);
    } else {
        SendHttpResphone(req, value.strValue);
    }
}


