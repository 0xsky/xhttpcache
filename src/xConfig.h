/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/

 
#ifndef _X_CONFIG_H_
#define _X_CONFIG_H_

#include "xIniFile.h"


typedef struct _XHTTP_CACHE_CONFIG_
{
    string    log_filename;
    uint64_t  log_filesize;
    string    log_level;
    int       run_daemonize;
    int       run_guard;
    int       debug;

    string    redis_ip;
    int       redis_port;
    string    redis_passwd;
} CACHE_CONFIG, *PCACHE_CONFIG;

typedef struct _XHTTP_CACHE_CONFIG_HTTPD_FRONTEND
{
    string    ip;
    int       port;
    int       threads;
    int       timeout;
    int       gzip;
    int       etag;
    int       etag_cache_count;
    int       etag_cache_size;
    string    pemfile;
    string    privfile;
    int       ssl_timeout;
    string    default_index;
} HTTP_FRONTEND_CONFIG, *PHTTP_FRONTEND_CONFIG;

typedef struct _XHTTP_CACHE_CONFIG_HTTPD_BACKEND
{
    string    ip;
    int       port;
    int       threads;
    int       timeout;
    string    username;
    string    password;
    string    encode_pass;
    string    pemfile;
    string    privfile;
    int       ssl_timeout;
    
    int       max_upload_file_size; //KB
} HTTP_BACKEND_CONFIG, *PHTTP_BACKEND_CONFIG;

typedef struct _ROCKSDB_CONFIG_
{
    std::string db_base_dir;
    bool        create_if_missing;
    int         max_open_files;
    int         max_file_opening_threads;
    uint64_t    max_total_wal_size;
    std::string db_log_dir;
    size_t      write_buffer_size;
    bool        compression;
    uint32_t    ttl;
} ROCKSDB_CONFIG, *PROCKSDB_CONFIG;


class xConfig
{
private:
    xConfig(){}
    virtual ~xConfig(){}

public:
    static xConfig *GetInstance();

    bool Load(const std::string &inifile);
    bool LoadxHttpdAppConfig(CIniFile &iniFile);
    bool LoadxDBConfig(CIniFile &iniFile);
    void Debug();
public:
    CACHE_CONFIG         app;
    HTTP_FRONTEND_CONFIG frontend;
    HTTP_BACKEND_CONFIG  backend;
    ROCKSDB_CONFIG       db;
};


#endif
