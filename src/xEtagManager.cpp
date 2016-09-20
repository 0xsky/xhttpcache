/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/


#include "xEtagManager.h"

void EtagCache::Set(uint64_t etagkey)
{
    //log_debug("set etagkey:%llu ", etagkey);
    XLOCK(etaglock);
    if (etagmap.find(etagkey) == etagmap.end()) {
        //log_debug("etagList capacity:%u size %lu ", capacity, etaglist.size());
        if (etaglist.size() == capacity) {
            //删除链表尾部节点（最少访问的节点）
            etagmap.erase(etaglist.back());
            etaglist.pop_back();
        }
        //插入新节点到链表头部,并且更新map中增加该节点
        etaglist.push_front(etagkey);
        //log_debug("size etagList:%lu ", etaglist.size());
        etagmap[etagkey] = etaglist.begin();
        //log_debug("set etagkey: %llu ", etagkey);
    } else {
        //更新节点的值，把当前访问的节点移到链表头部,并且更新map中该节点的地址
        etaglist.splice(etaglist.begin(), etaglist, etagmap[etagkey]);
        etagmap[etagkey] = etaglist.begin();
    }
}

bool EtagCache::Get(uint64_t etagkey)
{
    log_debug("get %llu ", etagkey);
    
    XLOCK(etaglock);
    if (etagmap.find(etagkey) == etagmap.end()) {
        return false;
    } else {
        //把当前访问的节点移到链表头部,并且更新map中该节点的地址
        etaglist.splice(etaglist.begin(), etaglist, etagmap[etagkey]);
        etagmap[etagkey] = etaglist.begin();
        return true;
    }
}


void xEtagManager::Init(size_t cnt, size_t size)
{
    count = cnt;
    etag_cache = new EtagCache[count];
    for (size_t i = 0; i < count; ++i) {
        etag_cache[i].SetSize(size);
    }
}

bool xEtagManager::GetKey(const std::string &key, uint64_t timestamp)
{
    uint64_t hash = MurmurHash64A(key.c_str(), key.length(), MURMUR_SEED);
    uint64_t hashkey = hash + timestamp;
    int cache_index = hashkey %count;
    log_debug("getkey %s %llu %llu %d", key.c_str(), hash, timestamp, cache_index);
    total_times++;
    bool bRet = etag_cache[cache_index].Get(hashkey);
    if (bRet) {
        ++get_hit;
    }
    return bRet;
}

void xEtagManager::SetKey(const std::string &key, uint64_t timestamp)
{
    // todo 这里 hashkey 的处理需要再优化以减少碰撞
    uint64_t hash = MurmurHash64A(key.c_str(), key.length(), MURMUR_SEED);
    uint64_t hashkey = hash + timestamp;
    int cache_index = hashkey %count;
    //log_debug("setkey %s %llu %llu %d ", key.c_str(), hash, timestamp, cache_index);
    etag_cache[cache_index].Set(hashkey);
}


