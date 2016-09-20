/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/


#include "xRedisServer.h"
#include "xLog.h"

xRedisServer::xRedisServer()
{
    xdb = NULL;
    expiremgr = NULL;
}

xRedisServer::~xRedisServer()
{

}

bool xRedisServer::Init(xDataBase *db)
{
    SystemCmdRegister();
    xdb = db;
    expiremgr = new ExpireManager;
    expiremgr->Init(xdb);
    expiremgr->StartTTL();
    return true;
}

bool xRedisServer::SystemCmdRegister()
{
    if (!SetCmdTable("set", (CmdCallback)&xRedisServer::ProcessCmd_set)) return false;
    if (!SetCmdTable("get", (CmdCallback)&xRedisServer::ProcessCmd_get)) return false;
    if (!SetCmdTable("del", (CmdCallback)&xRedisServer::ProcessCmd_del)) return false;
    if (!SetCmdTable("expire", (CmdCallback)&xRedisServer::ProcessCmd_expire)) return false;
    if (!SetCmdTable("ttl", (CmdCallback)&xRedisServer::ProcessCmd_ttl)) return false;
    if (!SetCmdTable("scan", (CmdCallback)&xRedisServer::ProcessCmd_scan)) return false;
    if (!SetCmdTable("flushall", (CmdCallback)&xRedisServer::ProcessCmd_flushall)) return false;


    return true;
}

void xRedisServer::ProcessCmd_get(xRedisClient *pClient)
{
    if (2!=pClient->argc) {
        SendErrReply(pClient, "arg error", "arg error");
        return;
    }
    xValue val;
    std::string key(pClient->argv[1], sdslen(pClient->argv[1])); 
    int ret = xdb->Get(key, val);
    if (db_ok == ret) {
        SendBulkReply(pClient, val.str());
        return;
    }
    if (db_null == ret) {
        SendNullReply(pClient);
        return;
    }

    return;
}

void xRedisServer::ProcessCmd_set(xRedisClient *pClient)
{
    std::string key;
    std::string val;
    if(3==pClient->argc) {
        key.assign(pClient->argv[1], sdslen(pClient->argv[1]));
        val.assign(pClient->argv[2], sdslen(pClient->argv[2]));
        xdb->Set(key, val);
    } else if ((5 == pClient->argc) && (0 == strcasecmp("ex", pClient->argv[3]))) {
        key.assign(pClient->argv[1], sdslen(pClient->argv[1]));
        val.assign(pClient->argv[2], sdslen(pClient->argv[2]));
        int64_t ttl_time_ms = str_to_int64(pClient->argv[4]);
        int64_t expired = time_ms() + ttl_time_ms * 1000;
        char ttl_data[30] = {0};
        snprintf(ttl_data, sizeof(ttl_data), "%" PRId64, expired);
        //log_debug("ProcessCmd_set ttl_time_ms:%lld expired:%lld", ttl_time_ms, expired);

        xdb->Set(key, val);
        expiremgr->SetTTL(key, expired);
        int ret = xdb->Zset(expiremgr->expire_key, key, ttl_data);
        if (db_ok==ret) {
            expiremgr->SetTTL(key, expired);
        } else {
            SendNullReply(pClient);
            return ;
        }
    } else {
        SendErrReply(pClient, "set error ", "argc error");
        return ;
    }
    
    SendStatusReply(pClient, "OK");
    return ;
}

void xRedisServer::ProcessCmd_del(xRedisClient *pClient)
{
    log_debug("ProcessCmd_del argc:%d", pClient->argc);
    if (2 != pClient->argc) {
        SendErrReply(pClient, "del error", "arg error");
        return;
    }
    int ret_count = 0;
    for (int i = 1; i < pClient->argc; ++i) {
        std::string key(pClient->argv[i], sdslen(pClient->argv[i]));
        int ret = xdb->Del(key);
        ret_count = (db_ok == ret) ? (ret_count + 1) : (ret_count);
    }
    SendIntReply(pClient, ret_count);
    return;
}

void xRedisServer::ProcessCmd_expire(xRedisClient *pClient)
{
    if (3 != pClient->argc) {
        SendErrReply(pClient, "expire error", "arg error");
        return;
    }

    std::string key(pClient->argv[1], sdslen(pClient->argv[1]));
    std::string ttl(pClient->argv[2], sdslen(pClient->argv[2]));
    int64_t ttl_time_ms = str_to_int64(ttl);
    int64_t expired = time_ms() + ttl_time_ms * 1000;

    log_debug("ProcessCmd_expire ttl_time_ms:%lld expired:%lld", ttl_time_ms, expired);

    char ttl_data[30] = {0};
    int size = snprintf(ttl_data, sizeof(ttl_data), "%" PRId64, expired);
    if (size <= 0){
        log_error("snprintf return error!");
        return;
    }

    xValue val;
    int ret = xdb->Get(key, val);
    if (ret==db_ok) {
        ret = xdb->Zset(expiremgr->expire_key, key, ttl_data);
        if (db_ok==ret) {
            expiremgr->SetTTL(key, expired);
            SendIntReply(pClient, 1);
            return;
        }
    }

    SendIntReply(pClient, 0);
    return;
}

void xRedisServer::ProcessCmd_ttl(xRedisClient *pClient)
{
    if (2 != pClient->argc) {
        SendErrReply(pClient, "ttl error", "arg error");
        return;
    }

    std::string key(pClient->argv[1], sdslen(pClient->argv[1]));
    std::string score;
    int ret = xdb->Zget(expiremgr->expire_key, key, score);
    if (db_ok == ret) {
        int64_t ex = str_to_int64(score);
        int64_t ttl = (ex - time_ms()) / 1000;
        SendIntReply(pClient, ttl);
        return;
    } else if(db_null==ret) {
        SendIntReply(pClient, -2);
        return;
    }

    SendIntReply(pClient, 0);
    return;
}

void xRedisServer::ProcessCmd_scan(xRedisClient *pClient)
{
    std::string start;
    std::string end;
    char cEnd = 255;
    bool del_result = false;
    int limit = 0;
    int argc = pClient->argc;

    //log_info("processCmd_scan argc:%d", argc);
    if ((5==argc)&&(0==strcasecmp("limit", pClient->argv[argc-2]))) {
        start.assign(pClient->argv[1], sdslen(pClient->argv[1]));
        end.assign(pClient->argv[2], sdslen(pClient->argv[2]));
        limit = atoi(pClient->argv[argc-1]);
    } else if((4==argc)&&(0==strcasecmp("limit", pClient->argv[argc-2]))) {
        start.assign(pClient->argv[1], sdslen(pClient->argv[1]));
        end = cEnd;
        limit = atoi(pClient->argv[argc-1]);
    } else if ((4 == argc) && (0 == strcasecmp("del", pClient->argv[3]))) {
        start.assign(pClient->argv[1], sdslen(pClient->argv[1]));
        end.assign(pClient->argv[2], sdslen(pClient->argv[2]));
        limit = INT_MAX;
        del_result = true;
    } else if(3==argc) {
        start.assign(pClient->argv[1], sdslen(pClient->argv[1]));
        end.assign(pClient->argv[2], sdslen(pClient->argv[2]));
        limit = INT_MAX;
    } else if(2==argc) {
        start.assign(pClient->argv[1], sdslen(pClient->argv[1]));
        end = cEnd;
        limit = INT_MAX;
    } else {
        log_error("processCmd_scan argc:%d", argc);
        SendErrReply(pClient, "scan error", "arg error");
        return;
    }

    std::vector<std::string> vResult;
    int ret = xdb->ScanKey(start, end, limit, vResult);
    if (ret<0) {
        SendErrReply(pClient, "xdb error", "scankey error");
        return;
    }

    if (del_result) {
        size_t ret_count = vResult.size();
        for (size_t i = 0; i < ret_count; ++i) {
            xdb->Del(vResult[i]);
        }
        SendIntReply(pClient, ret_count);
        return;
    }

    //log_info("processCmd_scan size:%zu", vResult.size());
    SendMultiBulkReply(pClient, vResult);
    return;
}

void xRedisServer::ProcessCmd_flushall(xRedisClient *pClient)
{
    log_warn("processCmd_flushall \n");
    int ret = xdb->Flushall();
    if (db_ok != ret) {
        return;
    }

    SendStatusReply(pClient, "OK");
    return;
}





