/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/


#ifndef  _XREDISD_H_
#define  _XREDISD_H_

#include "xRedisServerLib.h"
#include "xDataBase.h"
#include "xExpireManager.h"


class xRedisClient :public xRedisConnectorBase
{
public:
    xRedisClient();
    ~xRedisClient();
private:
};

class xRedisServer:public xRedisServerBase
{
public:
    xRedisServer();
    ~xRedisServer();

public:
    bool Init(xDataBase *db);

private:
    bool SystemCmdRegister();
    void ProcessCmd_set(xRedisClient *pClient);
    void ProcessCmd_get(xRedisClient *pClient);
    void ProcessCmd_del(xRedisClient *pClient);
    void ProcessCmd_expire(xRedisClient *pClient);
    void ProcessCmd_ttl(xRedisClient *pClient);
    
    void ProcessCmd_scan(xRedisClient *pClient);
    void ProcessCmd_flushall(xRedisClient *pClient);

private:
    xDataBase *xdb;
    ExpireManager *expiremgr;
};


#endif


