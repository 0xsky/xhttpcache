/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/


#ifndef _EXPIREMANAGER_H_
#define _EXPIREMANAGER_H_

#include "xLog.h"
#include "xDataBase.h"
#include "sorted_set.h"

#define  BATCH_SIZE 1000

static inline int64_t time_ms(){
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000 + now.tv_usec / 1000;
}

class ExpireManager
{
public:
	ExpireManager();
	~ExpireManager();

	bool Init(xDataBase *db);
    bool StartTTL();

private:
    static void *Dispatch(void *arg);
    void ExpireLoop();
    void LoadExpirationKeys(int num);

public:
    int SetTTL(const std::string &key, int64_t ttl);

public:
    string    expire_key;
private:
	SortedSet sort_set;
    bool      brun;
    int64_t   first_timeout;
    xDataBase *xdb;
    
};


#endif

