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
#include <string.h>
#include "xLog.h"
#include "json.h"
#include "xConfig.h"
#include "xevHtpdBackend.h"
#include "Parser.h"
#include "xTimerCount.h"

xevHtpdBackend::xevHtpdBackend()
{
    xdb = NULL;
    sysdb = NULL;
}

xevHtpdBackend::~xevHtpdBackend()
{
}

bool xevHtpdBackend::Init(xDataBase *db) {
    if (db) {
        xdb = db;
        return true;
    }
    return false;
}

bool xevHtpdBackend::InitSysDB()
{
    ROCKSDB_CONFIG cfg;
    cfg.db_base_dir = "./xdb";
    cfg.create_if_missing = 1;
    cfg.max_open_files = 10240;
    cfg.max_file_opening_threads = 2;
    cfg.max_total_wal_size = 0;
    cfg.db_log_dir = "xdblog";
    cfg.write_buffer_size = 32;
    cfg.compression = 1;

    sysdb = new xDataBase;
    sysdb->SetEtag(xdb->GetEtag(), false);
    return sysdb->OpenDB(cfg);
}

bool xevHtpdBackend::Start(const char *ip, int port, int threads)
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


bool xevHtpdBackend::SetRouteTable(evhtp_t * http)
{
    log_info("SetRouteTable\n");

    struct timeval timeo;
    timeo.tv_sec = xConfig::GetInstance()->backend.timeout;
    timeo.tv_usec = 0;
    evhtp_set_timeouts(http, &timeo, &timeo);

    if (!evhtp_set_cb(http, "/scan", xevHtpdBackend::KeyScanCallback, this))    return false;
    if (!evhtp_set_cb(http, "/statistics", xevHtpdBackend::StatisticsCallback, this)) return false;
    if (!evhtp_set_cb(http, "/config", xevHtpdBackend::ConfigCallback, this)) return false;
    if (!evhtp_set_cb(http, "/admin.htm", xevHtpdBackend::AdminShellCallback, this)) return false;
    evhtp_set_gencb(http, xevHtpdBackend::RESTCallback, this);

    return true;
}

bool xevHtpdBackend::CheckAuth(evhtp_request_t *req)
{
    if (xConfig::GetInstance()->backend.encode_pass.empty()) {
        return true;
    }

    const char* auth = evhtp_header_find(req->headers_in, "Authorization");
    if (NULL==auth) {
        return false;
    }

    const char *pkey = xConfig::GetInstance()->backend.encode_pass.c_str();
    const char *p = strstr(auth, "Basic ") ;
    if (NULL==p) {
        return false;
    }
    p += strlen("Basic ");

    return (0==strcmp(p, (char*)pkey));
}

bool xevHtpdBackend::CheckSession(evhtp_request_t *req)
{
    if (!xevHtpdBackend::CheckAuth(req)) {
        AddHeader(req, "Content-Length", "0");
        AddHeader(req, "WWW-Authenticate", "Basic realm=\"xSky\"");
        AddHeader(req, "Server", req->htp->server_name);
        evhtp_send_reply(req, EVHTP_RES_UNAUTH);
        return false;
    }
    return true;
}

void xevHtpdBackend::RESTfulPro(evhtp_request_t *req, xDataBase *xdb)
{
    const char *uri = (char*)req->uri->path->full + 1;
    std::string key((const char*)uri);

    if (req->method == htp_method_GET) {
        xValue sval;
        int ret = xdb->Get(key, sval);
        if (ret == db_null) {
            SendStatusResphone(req, EVHTP_RES_NOTFOUND, "NOT FOUND");
            return;
        }

        SendHttpResphone(req, sval.str());
        return;
    } else if (req->method == htp_method_POST || req->method == htp_method_PUT) {
        const char* content_type = evhtp_header_find(req->headers_in, "Content-Type");
        const char* content_length = evhtp_header_find(req->headers_in, "Content-Length");

        size_t len = evbuffer_get_length(req->buffer_in);
        std::string value;
        char *buffer = (char *)malloc(len + 1);
        evbuffer_copyout(req->buffer_in, buffer, len);
        buffer[len] = 0;

        if ((content_type)&&strstr(content_type, "multipart/form-data")) {
            ParasrMultipartPost(req, buffer, len, value);
        } else {
            value = buffer;
        }

        //log_debug("RESTfulPro type:%d key:%s length:%d\n", req->method, key.c_str(), value.length());

        int ret = xdb->Set(key, value);
        free(buffer);
        if (ret > 0) {
            SendStatusResphone(req, EVHTP_RES_SERVERR, "EVHTP_RES_SERVERR");
            return;
        }
        SendStatusResphone(req, EVHTP_RES_OK, "OK");
    } else if (req->method == htp_method_DELETE) {
        int ret = xdb->Del(key);
        if (ret > 0) {
            SendStatusResphone(req, EVHTP_RES_NOTFOUND, "NOT FOUND");
            return;
        }
        SendStatusResphone(req, EVHTP_RES_ACCEPTED, "OK");
    } else {
        SendStatusResphone(req, EVHTP_RES_BADREQ, "BADREQUEST");
    }
}

void xevHtpdBackend::RESTCallback(evhtp_request_t *req, void *arg)
{
    TIME_TEST;
    xevHtpdBackend *pHttpd = reinterpret_cast<xevHtpdBackend *>(arg);
    const char *uri = (char*)req->uri->path->full + 1;

    xevHtpdBase::HttpDebug(req, xConfig::GetInstance()->app.debug);
    const char *type = GuessContentType(uri);
    AddHeader(req, "Content-Type", type);

    if (!xevHtpdBackend::CheckSession(req)) {
        return;
    }

    if (0 == strlen(uri)) {
        SendStatusResphone(req, EVHTP_RES_NOTFOUND, "NOT FOUND");
        return;
    }

    xDataBase *xdb = pHttpd->xdb;
    const char *pdb = uri_arg_find(req, "db");
    if ((NULL != pdb) && (0 == strcmp(pdb, "xdb"))) {
        xdb = pHttpd->sysdb;
    }

    RESTfulPro(req, xdb);
}

void xevHtpdBackend::AdminShellCallback(evhtp_request_t *req, void *arg)
{
    TIME_TEST;
    xevHtpdBackend *pHttpd = reinterpret_cast<xevHtpdBackend *>(arg);
    xevHtpdBase::HttpDebug(req, xConfig::GetInstance()->app.debug);
    const char *uri = (char*)req->uri->path->full + 1;

    if (!xevHtpdBackend::CheckSession(req)) {
        return;
    }

    const char *type = GuessContentType(uri);
    AddHeader(req, "Content-Type", type);

    //log_debug("RestCallback uri:%s type:%s", uri, type);
    RESTfulPro(req, pHttpd->sysdb);
}


void xevHtpdBackend::StatisticsCallback(evhtp_request_t *req, void *arg)
{
    TIME_TEST;
    xevHtpdBackend *pHttpd = reinterpret_cast<xevHtpdBackend *>(arg);
    xevHtpdBase::HttpDebug(req, xConfig::GetInstance()->app.debug);
    log_debug("xevHtpdBackend::StatisticsCallback \r\n");

    if (!xevHtpdBackend::CheckSession(req)) {
        return;
    }

    std::string strResult;
    pHttpd->xdb->StatusStr(strResult);

    AddHeader(req, "Content-Type", "text/plain");
    xevHtpdBackend::SendHttpResphone(req, strResult);
}

void xevHtpdBackend::ConfigCallback(evhtp_request_t *req, void *arg)
{
    TIME_TEST;
    xevHtpdBase::HttpDebug(req, xConfig::GetInstance()->app.debug);
    log_debug("xevHtpdBackend::ConfigCallback \r\n");

    if (!xevHtpdBackend::CheckSession(req)) {
        return;
    }

    std::string strResult;
    Json::Value root;
    Json::Value frontend;
    Json::Value backend;
    Json::Value httpcache;
    Json::Value redis;
    Json::Value db;

    httpcache["logfilename"] = xConfig::GetInstance()->app.log_filename;
    httpcache["logfilesize"] = (uint32_t)xConfig::GetInstance()->app.log_filesize;
    httpcache["loglevel"] = xConfig::GetInstance()->app.log_level;
    httpcache["daemonize"] = xConfig::GetInstance()->app.run_daemonize;
    httpcache["guard"] = xConfig::GetInstance()->app.run_guard;
    httpcache["debug"] = xConfig::GetInstance()->app.debug;

    redis["ip"] = xConfig::GetInstance()->app.redis_ip;
    redis["port"] = xConfig::GetInstance()->app.redis_port;

    frontend["ip"] = xConfig::GetInstance()->frontend.ip;
    frontend["port"] = xConfig::GetInstance()->frontend.port;
    frontend["threads"] = xConfig::GetInstance()->frontend.threads;
    frontend["timeout"] = xConfig::GetInstance()->frontend.timeout;
    frontend["gzip"] = xConfig::GetInstance()->frontend.gzip;
    frontend["etag"] = xConfig::GetInstance()->frontend.etag;
    frontend["etagcachecount"] = xConfig::GetInstance()->frontend.etag_cache_count;
    frontend["etagcachesize"] = xConfig::GetInstance()->frontend.etag_cache_size;
    frontend["pemfile"] = xConfig::GetInstance()->frontend.pemfile;
    frontend["privfile"] = xConfig::GetInstance()->frontend.privfile;
    frontend["ssltimeout"] = xConfig::GetInstance()->frontend.ssl_timeout;
    frontend["defaultindex"] = xConfig::GetInstance()->frontend.default_index;

    backend["ip"] = xConfig::GetInstance()->backend.ip;
    backend["port"] = xConfig::GetInstance()->backend.port;
    backend["threads"] = xConfig::GetInstance()->backend.threads;
    backend["timeout"] = xConfig::GetInstance()->backend.timeout;
    backend["username"] = xConfig::GetInstance()->backend.username;
    backend["password"] = xConfig::GetInstance()->backend.password;

    backend["pemfile"] = xConfig::GetInstance()->backend.pemfile;
    backend["privfile"] = xConfig::GetInstance()->backend.privfile;
    backend["ssltimeout"] = xConfig::GetInstance()->backend.ssl_timeout;

    db["db_base_dir"] = xConfig::GetInstance()->db.db_base_dir;
    db["create_if_missing"] = xConfig::GetInstance()->db.create_if_missing;
    db["max_open_files"] = xConfig::GetInstance()->db.max_open_files;
    db["max_file_opening_threads"] = xConfig::GetInstance()->db.max_file_opening_threads;
    db["max_total_wal_size"] = (uint32_t)xConfig::GetInstance()->db.max_total_wal_size;
    db["db_log_dir"] = xConfig::GetInstance()->db.db_log_dir;
    db["write_buffer_size"] = (uint32_t)xConfig::GetInstance()->db.write_buffer_size;
    db["compression"] = xConfig::GetInstance()->db.compression;
    db["ttl"] = xConfig::GetInstance()->db.ttl;

    root["frontend"] = frontend;
    root["backend"] = backend;
    root["httpcache"] = httpcache;
    root["redis"] = redis;
    root["db"] = db;

    Json::FastWriter writer;
    strResult = writer.write(root);

    AddHeader(req, "Content-Type", "text/json");
    xevHtpdBackend::SendHttpResphone(req, strResult);
}

void xevHtpdBackend::KeyScanCallback(evhtp_request_t *req, void *arg)
{
    xevHtpdBackend *pHttpd = reinterpret_cast<xevHtpdBackend *>(arg);
    TIME_TEST;
    xevHtpdBase::HttpDebug(req, xConfig::GetInstance()->app.debug);
    
    int ret = 0;
    //log_debug("xevHtpdBackend::KeyScanCallback \r\n");
    if (!xevHtpdBackend::CheckSession(req)) {
        return;
    }

    const char *pkey = uri_arg_find(req, "key");
    const char *plimit = uri_arg_find(req, "limit");
    if (NULL == plimit) {
        plimit = "1000";
    }
    
    xDataBase *xdb = pHttpd->xdb;
    const char *pdb = uri_arg_find(req, "db");
    if ((NULL != pdb) && (0 == strcmp(pdb, "xdb"))) {
        //log_debug("RestCallback pdb:%s", pdb);
        xdb = pHttpd->sysdb;
    }

    char cEnd = 255;
    std::string start;
    std::string end;
    if (pkey) {
        start = pkey;
    }
    end = cEnd;
    int limit = atoi(plimit);
    std::vector<std::string> vkey;
    ret = xdb->ScanKey(start, end, limit, vkey);
    if (ret < 0) {
        xevHtpdBackend::SendStatusResphone(req, EVHTP_RES_NOTFOUND, "NOT FOUND %s", req->uri);
        log_error("scan error \r\n");
        return;
    }
    //log_debug("xevHtpdBackend::KeyScanCallback %d \r\n", vkey.size());
    Json::Value keylist;
    std::vector<std::string>::iterator iter = vkey.begin();
    for (; iter != vkey.end(); ++iter) {
        keylist.append(*iter);
    }

    Json::FastWriter writer;
    std::string strJson = writer.write(keylist);
    AddHeader(req, "Content-Type", "text/json; charset=UTF-8");
    xevHtpdBackend::SendHttpResphone(req, strJson);
}

bool xevHtpdBackend::ParasrMultipartPost(evhtp_request_t *req, const char* buf, int length, std::string &filedata)
{
    const char* content_type = evhtp_header_find(req->headers_in, "Content-Type");
    if (NULL != content_type) {
        log_debug("ParasrMultipartPost type:%d content_type:%s\n", req->method, content_type);
    }
    try {
        MPFD::Parser *POSTParser = new MPFD::Parser();
        POSTParser->SetUploadedFilesStorage(MPFD::Parser::StoreUploadedFilesInMemory);
        //POSTParser->SetTempDirForFileUpload("/tmp");
        POSTParser->SetMaxCollectedDataLength(xConfig::GetInstance()->backend.max_upload_file_size);
        POSTParser->SetContentType(content_type);
        POSTParser->AcceptSomeData(buf, length);

        // Now see what we have:
        std::map<std::string, MPFD::Field *> fields = POSTParser->GetFieldsMap();

        std::cout << "Have " << fields.size() << " fields\n\r";

        std::map<std::string, MPFD::Field *>::iterator it;
        for (it = fields.begin(); it != fields.end(); it++) {
            if (fields[it->first]->GetType() == MPFD::Field::TextType) {
                std::cout << "Got text field: [" << it->first << "], value: [" << fields[it->first]->GetTextTypeContent() << "]\n";
            } else {
                std::cout << "Got file field: [" << it->first << "] Filename:[" << fields[it->first]->GetFileName() << "] \n";
                log_info("UploadCallback GetFileName:%s GetFileContentSize:%d \r\n",
                    fields[it->first]->GetFileName().c_str(),
                    fields[it->first]->GetFileContentSize());

                filedata.assign(fields[it->first]->GetFileContent(), fields[it->first]->GetFileContentSize());
                return true;
            }
        }

    } catch (MPFD::Exception e) {
        log_error("ParasrMultipartPost  error:%s \r\n", e.GetError().c_str());
        return false;
    }
    return false;
}








