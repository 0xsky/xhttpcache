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
#include "xevHtpdBase.h"

xevHtpdBase::xevHtpdBase()
{
    base = event_base_new();
    http = evhtp_new(base, NULL);
    usessl = false;
    sslcfg = NULL;
}

xevHtpdBase::~xevHtpdBase()
{
    event_base_free(base);
    evhtp_free(http);
    if (sslcfg) {
        if (sslcfg->pemfile) {
            free(sslcfg->pemfile);
        }
        if (sslcfg->privfile) {
            free(sslcfg->privfile);
        }
        delete sslcfg;
    }
}

bool xevHtpdBase::InitSSL(const char *pemfile, const char*privfile, int timeout)
{
    if ((0==strlen(pemfile))||(0==strlen(privfile))){
        log_info("InitSSL: do not use ssl");
        return false;
    }
    usessl = true;
    sslcfg = new evhtp_ssl_cfg_t;
    sslcfg->pemfile = strdup(pemfile);
    sslcfg->privfile = strdup(privfile);
    sslcfg->scache_type = evhtp_ssl_scache_type_internal;
    sslcfg->scache_timeout = timeout;

    evhtp_ssl_init(http, sslcfg);
    log_info("InitSSL: use ssl");
    return true;
}

bool xevHtpdBase::BindSocket(const char *ip, int port)
{
    log_debug("BindSocket: %s:%d \r\n", ip, port);

    http->enable_nodelay = 1;
    http->enable_defer_accept = 1;
    http->enable_reuseport = 1;
    http->server_name = strdup(SERVER_SIGNATURE);

    evhtp_bind_socket(http, ip, port, 1024);
    return true;
}

bool xevHtpdBase::Run(int nthreads)
{
    int ret = evhtp_use_threads_wexit(http, NULL, NULL, nthreads, this);
    log_info("xevHtpdBase evhtp_use_threads ret:%d", ret);
    std::thread thd = std::thread(xevHtpdBase::Dispatch, base);
    thd.detach();
    return true;
}

void *xevHtpdBase::Dispatch(void *arg){
    if (NULL == arg) {
        log_error("error arg is null\n");
        return NULL;
    }
    log_info("xevHtpdBase thread start\n");
    event_base_dispatch((struct event_base *) arg);
    log_info("xevHtpdBase thread end\n");
    return NULL;
}

void xevHtpdBase::OnTimer()
{
    
}

const char *xevHtpdBase::GuessContentType(const char *path) {

    static const struct table_entry {
    const char *extension;
    const char *content_type;
    } content_type_table[] = {
        { "htm", "text/html" },
        { "js",  "application/x-javascript" },
        { "html","text/html" },
        { "css", "text/css" },
        { "txt", "text/plain" },
        { "c", "text/plain" },
        { "cpp", "text/plain" },
        { "h", "text/plain" },
        { "gif", "image/gif" },
        { "jpg", "image/jpeg" },
        { "ico", "image/gif" },
        { "jpeg","image/jpeg" },
        { "png", "image/png" },
        { "pdf", "application/pdf" },
        { "ps",  "application/postsript" },
        { NULL, NULL },
    };

    const char *last_period, *extension;
    const struct table_entry *ent;
    const char *type = "text/plain";
    last_period = strrchr(path, '.');
    if (!last_period || strchr(last_period, '/')) {
        
    } else {
        extension = last_period + 1;
        for (ent = &content_type_table[0]; ent->extension; ++ent) {
            if (!stricmp(ent->extension, extension)) {
                type = ent->content_type;
            }
        }
    }
    return type;
}

void xevHtpdBase::HttpDebug(evhtp_request_t *req,int debug) {
    (void)req;
    if (0==debug){
        return;
    }
    log_debug("method:%d methodstr:%s", evhtp_request_get_method(req), htparser_get_methodstr_m(req->method));
    log_debug("full:%s", req->uri->path->full);
    log_debug("path:%s", req->uri->path->path);
    log_debug("file:%s", req->uri->path->file);
    log_debug("match_start:%s", req->uri->path->match_start);
    log_debug("match_end:%s", req->uri->path->match_end);
    xevHtpdBase::DebugHttpHeader(req);
}

int xevHtpdBase::dump_header(evhtp_kv_t * kv, void * arg)
{
    (void)arg;
    log_debug("dump_header %s: %s\n", kv->key, kv->val);
    return 0;
}

void xevHtpdBase::DebugHttpHeader(evhtp_request_t *req) {
    evhtp_kvs_for_each(req->headers_in, dump_header, (void *)0);
}

const char *xevHtpdBase::uri_arg_find(evhtp_request_t  *req, const char *arg)
{
    const char *val = NULL;
    if (req&&req->uri&&req->uri->query) {
        evhtp_kv_s * kv = req->uri->query->tqh_first;
        int len = strlen(arg);
        while (kv) {
            if ((0 == strncmp(arg, kv->key, len)) && kv->val && kv->vlen > 0) {
                val = kv->val;
                break;
            }
            kv = kv->next.tqe_next;
        }
    }

    return val;
}

// 使用完需要调用 evhtp_query_free(kvs); 
evhtp_query_t *xevHtpdBase::GetHttpPostText(evhtp_request_t *req, struct evkeyvalq *evdata) {
    (void)evdata;
    size_t len = evbuffer_get_length(req->buffer_in);
    char *buffer = (char *)malloc(len + 1);
    evbuffer_copyout(req->buffer_in, buffer, len);
    buffer[len] = 0;

    evhtp_query_t *kvs = evhtp_parse_query(buffer, len);
    free(buffer);

    return kvs;
}

char* xevHtpdBase::GetHttpPostData(evhtp_request_t *req)
{
    size_t len = evbuffer_get_length(req->buffer_in);
    char *buffer = (char *)malloc(len + 1);
    evbuffer_copyout(req->buffer_in, buffer, len);
    return buffer;
}

void xevHtpdBase::AddHeader(evhtp_request_t  *req, const char* headkey, const char* headval)
{
    evhtp_headers_add_header(req->headers_out, evhtp_header_new(headkey, headval, 0, 0));
}

void xevHtpdBase::SendStatusResphone(evhtp_request_t *req, int errcode, const char*fmt, ...)
{
    char szBuf[1024] = { 0 };
    va_list va;
    va_start(va, fmt);
    vsnprintf(szBuf, sizeof(szBuf), fmt, va);
    va_end(va);
    log_debug("%s", szBuf);
    
    evbuffer_add(req->buffer_out, szBuf, strlen(szBuf));
    evhtp_send_reply(req, errcode);
}

void xevHtpdBase::SendHttpResphone(evhtp_request_t *req, const string & strData) {
    AddHeader(req, "Server", req->htp->server_name);
    evbuffer_add(req->buffer_out, strData.c_str(), strData.length());
    evhtp_send_reply(req, EVHTP_RES_OK);
}

void xevHtpdBase::SendHttpResphone(evhtp_request_t *req, const char* data, int size) {
    AddHeader(req, "Server", req->htp->server_name);
    evbuffer_add(req->buffer_out, data, size);
    evhtp_send_reply(req, EVHTP_RES_OK);
}



