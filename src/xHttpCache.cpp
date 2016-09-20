/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/


#define XHTTPDB_VERSION "0.1"


#include <signal.h>
#include "xConfig.h"
#include "xLog.h"
#include "xRedisServer.h"
#include "xevHtpdBackend.h"
#include "xevHtpdFrontend.h"
#include "xEtagManager.h"

#define CONFIGFILE "xhttpcache.cnf"

const char* config_file = CONFIGFILE;
bool bRun = true;

void version()
{
    log_info(" ");
    log_info("    |_| _|_ _|_ ._   _  _.  _ |_   _  ");
    log_info(" >< | |  |_  |_ |_) (_ (_| (_ | | (/_ ");
    log_info("                |                     ");

    log_info("xHttpcache version=%s libevent=%s:%d bits=%d %s %s\n",
        XHTTPDB_VERSION,
        event_get_version(),
        (int)event_get_version_number(),
        sizeof(long) == 4 ? 32 : 64, __DATE__, __TIME__);
}

static int signal_ignore(int sig)
{
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;

    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(sig, &sa, 0) == -1) {
        return -1;
    }
    return 0;
}
static void signal_handler(int sig) {
    switch (sig) {
    case SIGTERM: break;
    case SIGINT:

        break;
    case SIGALRM:  break;
    case SIGHUP:   break;
    case SIGCHLD:  break;
    }

    log_error("sig:%d", sig);
    exit(-1);
}

bool signal_handle()
{
#ifndef WIN32
    signal(SIGPIPE, SIG_IGN);
    signal_ignore(SIGHUP);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGALRM, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP,  signal_handler);
    signal(SIGINT,  signal_handler);
#endif
    return true;
}

void xhttpcache()
{
    version();
    signal_handle();

    LOGGER.setLogLevel("DEBUG");
    xConfig::GetInstance()->Load(config_file);
    LOGGER.setFileName(xConfig::GetInstance()->app.log_filename.c_str(), true);
    LOGGER.setLogLevel(xConfig::GetInstance()->app.log_level.c_str());
    LOGGER.setMaxFileSize(xConfig::GetInstance()->app.log_filesize);
    version();
    xConfig::GetInstance()->Debug();

    xEtagManager *etag = NULL;
    xDataBase *xdb = new xDataBase;
    if (1==xConfig::GetInstance()->frontend.etag) {
        etag  = new xEtagManager;
        etag->Init(xConfig::GetInstance()->frontend.etag_cache_count, xConfig::GetInstance()->frontend.etag_cache_size);
        xdb->SetEtag(etag);
    }
    xdb->OpenDB(xConfig::GetInstance()->db);

    xevHtpdFrontend xhtpfrontend;
    xhtpfrontend.Init(xdb);
    xhtpfrontend.InitSSL(xConfig::GetInstance()->frontend.pemfile.c_str(),
        xConfig::GetInstance()->frontend.privfile.c_str(),
        xConfig::GetInstance()->frontend.ssl_timeout);
    xhtpfrontend.Start(xConfig::GetInstance()->frontend.ip.c_str(),
        xConfig::GetInstance()->frontend.port,
        xConfig::GetInstance()->frontend.threads);

    xevHtpdBackend xhtpbackend;
    xhtpbackend.Init(xdb);
    xhtpbackend.InitSysDB();
    xhtpfrontend.InitSSL(xConfig::GetInstance()->backend.pemfile.c_str(),
        xConfig::GetInstance()->backend.privfile.c_str(),
        xConfig::GetInstance()->backend.ssl_timeout);
    xhtpbackend.Start(xConfig::GetInstance()->backend.ip.c_str(),
        xConfig::GetInstance()->backend.port,
        xConfig::GetInstance()->backend.threads);

    xRedisServer xRedis;
    xRedis.Init(xdb);
    xRedis.Start(xConfig::GetInstance()->app.redis_ip.c_str(),
        xConfig::GetInstance()->app.redis_port);

    while (bRun) {
        SLEEP(3000);
    }
    if (xdb) {
        delete xdb;
    }
    
    if (etag) {
        delete etag;
    }
    
    log_info("xhttpcache end\r\n");
}

int main(int argc, char **argv)
{
    if (argc>=2){
        config_file = argv[1];
    }
    xConfig::GetInstance()->Load(config_file);
    if (1==xConfig::GetInstance()->app.run_daemonize) {
        daemonize();
    }

    if (1 == xConfig::GetInstance()->app.run_guard){
        guard(xhttpcache, 0);
    } else {
        xhttpcache();
    }

    return 0;
}


