/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/


#ifndef _X_ETAG_MANAGER_H_
#define _X_ETAG_MANAGER_H_

#include "MurmurHash2.h"
#include "xLock.h"
#include "xLog.h"
#include <string.h>
#include <list>
#include <string>
#include <unordered_map>
using namespace std;

#define MURMUR_SEED 0xEE6B27EB



class EtagCache
{
public:
    EtagCache(){

    }

    ~EtagCache(){

    }

    void SetSize(size_t size)
    {
        capacity = size;
    }

public:
    bool Get(uint64_t etagkey);

    void Set(uint64_t etagkey);

private:
    xLock                etaglock;
    size_t               capacity;
    std::list<uint64_t>  etaglist;
    unordered_map<uint64_t, std::list<uint64_t>::iterator> etagmap;

};


class xEtagManager
{
public:
    xEtagManager()
    {
        get_hit = 0;
        total_times = 0;
        count = 0;
        etag_cache = NULL;
    }
    ~xEtagManager()
    {
        delete etag_cache;
        etag_cache = NULL;
    }

public:
    void Init(size_t cnt, size_t size);
    bool GetKey(const std::string &key, uint64_t timestamp);
    void SetKey(const std::string &key, uint64_t timestamp);
    float GetHitRate()
    {
        return (0 == total_times) ? 0 : (get_hit / total_times);
    }
private:
    // 计算命中率
    uint32_t get_hit;
    uint32_t total_times;
private:
    size_t count;
    EtagCache *etag_cache;
};


#endif

