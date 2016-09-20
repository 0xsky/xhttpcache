/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/


#include "util.h"
#include "xConfig.h"
#include "xLog.h"
#include <algorithm>
 
xConfig *xConfig::GetInstance()
{
    static xConfig xcfig;
    return &xcfig;
}

bool xConfig::Load(const std::string &IniFile)
{
    CIniFile iniFile(IniFile);
    if (iniFile.ReadFile()){
        LoadxHttpdAppConfig(iniFile);
        LoadxDBConfig(iniFile);
        return true;
    }
    return false;
}

bool xConfig::LoadxHttpdAppConfig(CIniFile &iniFile) 
{
    app.redis_ip        = iniFile.GetValue("xhttpcache", "redis_ip");
    app.redis_port      = iniFile.GetValueI("xhttpcache", "redis_port");
    app.debug           = iniFile.GetValueI("xhttpcache", "debug");
    app.log_filename    = iniFile.GetValue("xhttpcache", "log_filename");
    app.log_filesize    = iniFile.GetValueI("xhttpcache", "log_filesize");
    app.log_level       = iniFile.GetValue("xhttpcache", "log_level");
    app.run_daemonize   = iniFile.GetValueI("xhttpcache", "daemonize");
    app.run_guard       = iniFile.GetValueI("xhttpcache", "guard");
    std::transform(app.log_level.begin(), app.log_level.end(), app.log_level.begin(), ::toupper);

    frontend.ip            = iniFile.GetValue("xhttpd_frontend",  "ip");
    frontend.port          = iniFile.GetValueI("xhttpd_frontend", "port");
    frontend.threads       = iniFile.GetValueI("xhttpd_frontend", "threads");
    frontend.timeout       = iniFile.GetValueI("xhttpd_frontend", "timeout");
    frontend.gzip          = iniFile.GetValueI("xhttpd_frontend", "gzip");
    frontend.etag          = iniFile.GetValueI("xhttpd_frontend", "etag", 1);
    frontend.etag_cache_count = iniFile.GetValueI("xhttpcache", "etag_cache_count", 4);
    frontend.etag_cache_size = iniFile.GetValueI("xhttpcache", "etag_cache_size", 1000);
    frontend.pemfile       = iniFile.GetValue("xhttpd_frontend",  "pemfile");
    frontend.privfile      = iniFile.GetValue("xhttpd_frontend",  "privfile");
    frontend.ssl_timeout   = iniFile.GetValueI("xhttpd_frontend", "ssl_timeout", 5);
    frontend.default_index = iniFile.GetValue("xhttpd_frontend",  "default_index");

    backend.ip          = iniFile.GetValue("xhttpd_backend",  "ip");
    backend.port        = iniFile.GetValueI("xhttpd_backend", "port");
    backend.threads     = iniFile.GetValueI("xhttpd_backend", "threads");
    backend.timeout     = iniFile.GetValueI("xhttpd_backend", "timeout");
    backend.username    = iniFile.GetValue("xhttpd_backend",  "username");
    backend.password    = iniFile.GetValue("xhttpd_backend",  "password");
    backend.pemfile     = iniFile.GetValue("xhttpd_backend",  "pemfile");
    backend.privfile    = iniFile.GetValue("xhttpd_backend",  "privfile");
    backend.ssl_timeout = iniFile.GetValueI("xhttpd_backend", "ssl_timeout", 5);
    backend.max_upload_file_size     = iniFile.GetValueI("xhttpd_backend", "max_upload_file_size", 10);

    if (frontend.etag_cache_size == 0) {
        frontend.etag_cache_size = 1000;
    }

    if (backend.max_upload_file_size>0) {
        backend.max_upload_file_size = backend.max_upload_file_size * 1024;
    } else {
        backend.max_upload_file_size = 128 * 1024;
    }

    if ((backend.username.length()>0)
        && (backend.password.length()>0)) {
        encode_pass(backend.username, backend.password, backend.encode_pass);
    }

    return true;
}

bool xConfig::LoadxDBConfig(CIniFile &iniFile)
{
    db.db_base_dir = iniFile.GetValue("rocksdb", "db_base_dir");
    db.create_if_missing = iniFile.GetValueB("rocksdb", "create_if_missing");
    db.max_open_files = iniFile.GetValueI("rocksdb", "max_open_files");
    db.max_file_opening_threads = iniFile.GetValueI("rocksdb", "max_file_opening_threads");
    db.max_total_wal_size = iniFile.GetValueI("rocksdb", "max_total_wal_size");
    db.db_log_dir = iniFile.GetValue("rocksdb", "db_log_dir");
    db.write_buffer_size = iniFile.GetValueI("rocksdb", "write_buffer_size");
    db.compression = iniFile.GetValueB("rocksdb", "compression");
    db.ttl = iniFile.GetValueB("rocksdb", "ttl");
    return true;
}

void xConfig::Debug()
{
    log_info("xhttpcache redis_ip:%s", xConfig::GetInstance()->app.redis_ip.c_str());
    log_info("xhttpcache redis_port:%d", xConfig::GetInstance()->app.redis_port);
    log_info("xhttpcache log_filename:%s", xConfig::GetInstance()->app.log_filename.c_str());
    log_info("xhttpcache log_filesize:%lld", xConfig::GetInstance()->app.log_filesize);
    log_info("xhttpcache log_level:%s", xConfig::GetInstance()->app.log_level.c_str());
    log_info("xhttpcache run_daemonize:%d", xConfig::GetInstance()->app.run_daemonize);
    log_info("xhttpcache run_guard:%d", xConfig::GetInstance()->app.run_guard);
    log_info("xhttpcache debug:%d", xConfig::GetInstance()->app.debug);
    

    log_info("httpd_frontend ip:%s", xConfig::GetInstance()->frontend.ip.c_str());
    log_info("httpd_frontend port:%d", xConfig::GetInstance()->frontend.port);
    log_info("httpd_frontend threads:%d", xConfig::GetInstance()->frontend.threads);
    log_info("httpd_frontend timeout:%d", xConfig::GetInstance()->frontend.timeout);
    log_info("httpd_frontend gzip:%d", xConfig::GetInstance()->frontend.gzip);
    log_info("httpd_frontend etag:%d", xConfig::GetInstance()->frontend.etag);
    log_info("httpd_frontend etag_cache_count:%d", xConfig::GetInstance()->frontend.etag_cache_count);
    log_info("httpd_frontend etag_cache_size:%d", xConfig::GetInstance()->frontend.etag_cache_size);
    log_info("httpd_frontend pemfile:%s", xConfig::GetInstance()->frontend.pemfile.c_str());
    log_info("httpd_frontend privfile:%s", xConfig::GetInstance()->frontend.privfile.c_str());
    log_info("httpd_frontend ssl_timeout:%d", xConfig::GetInstance()->frontend.ssl_timeout);
    log_info("httpd_frontend default_index:%s", xConfig::GetInstance()->frontend.default_index.c_str());
    

    log_info("httpd_backend ip:%s", xConfig::GetInstance()->backend.ip.c_str());
    log_info("httpd_backend port:%d", xConfig::GetInstance()->backend.port);
    log_info("httpd_backend threads:%d", xConfig::GetInstance()->backend.threads);
    log_info("httpd_backend timeout:%d", xConfig::GetInstance()->backend.timeout);
    log_info("httpd_backend username:%s", xConfig::GetInstance()->backend.username.c_str());
    log_info("httpd_backend password:%s", xConfig::GetInstance()->backend.password.c_str());
    log_info("httpd_backend pemfile:%s", xConfig::GetInstance()->backend.pemfile.c_str());
    log_info("httpd_backend privfile:%s", xConfig::GetInstance()->backend.privfile.c_str());
    log_info("httpd_backend ssl_timeout:%d", xConfig::GetInstance()->backend.ssl_timeout);
    log_info("httpd_backend max_upload_file_size:%d", xConfig::GetInstance()->backend.max_upload_file_size);

    log_info("db db_base_dir:%s", xConfig::GetInstance()->db.db_base_dir.c_str());
    log_info("db create_if_missing:%d", xConfig::GetInstance()->db.create_if_missing);
    log_info("db max_open_files:%d", xConfig::GetInstance()->db.max_open_files);
    log_info("db max_file_opening_threads:%d", xConfig::GetInstance()->db.max_file_opening_threads);
    log_info("db max_total_wal_size:%zu", xConfig::GetInstance()->db.max_total_wal_size);
    log_info("db db_log_dir:%s", xConfig::GetInstance()->db.db_log_dir.c_str());
    log_info("db write_buffer_size:%zu", xConfig::GetInstance()->db.write_buffer_size);
    log_info("db compression:%d", xConfig::GetInstance()->db.compression);
}



