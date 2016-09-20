/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/


#include "xExpireManager.h"
#include <thread>


ExpireManager::ExpireManager()
{
    xdb = NULL;
    brun = true;
    first_timeout = 0;
    expire_key = "expire";
}

bool ExpireManager::Init(xDataBase *db)
{
    if (db) {
        xdb = db;
        return true;
    }
    return false;
}

bool ExpireManager::StartTTL()
{
    log_info("ExpireManager StartTTL");
    std::thread thd = std::thread(ExpireManager::Dispatch, this);
    thd.detach();
    return true;
}

void *ExpireManager::Dispatch(void *arg){
    ExpireManager *pthis = reinterpret_cast<ExpireManager*>(arg);
    if (NULL == pthis) {
        log_error("error arg is null\n");
        return NULL;
    }

    log_info("ExpireManager thread start first_timeout:%lld \n", pthis->first_timeout);

    while (pthis->brun){
        if (pthis->first_timeout > time_ms()){
            usleep(10 * 1000);
            continue;
        }
        pthis->ExpireLoop();
    }

    log_debug("ExpirationHandler thread quit");
    pthis->brun = false;
    return NULL;
}

void ExpireManager::ExpireLoop(){

    if (this->sort_set.empty()){
        this->LoadExpirationKeys(BATCH_SIZE);
        if (this->sort_set.empty()){
            this->first_timeout = INT64_MAX;
            log_debug("expire_loop INT64_MAX:%d", first_timeout);
            return;
        }
    }

    int64_t score;
    std::string key;
    if (this->sort_set.front(&key, &score)){
        this->first_timeout = score;

        if (score <= time_ms()){
            log_debug("del expired %s", key.c_str());
            xdb->Del(key);
            xdb->Zdel(expire_key, key);
            this->sort_set.pop_front();
        }
    }
}

void ExpireManager::LoadExpirationKeys(int num){
    log_debug("load_expiration_keys_from_db %d", num);
    xdb->LoadExpirationKeys(expire_key,sort_set, num);
}

int ExpireManager::SetTTL(const std::string &key, int64_t ttl) {
    if (ttl < first_timeout){
        first_timeout = ttl;
    }
    if (!sort_set.empty() && ttl <= sort_set.max_score()){
        sort_set.add(key, ttl);
        if (sort_set.size() > BATCH_SIZE){
            sort_set.pop_back();
        }
    } else{
        sort_set.del(key);
        //log_debug(" ttl:%lld max_score:%lld ", ttl, sort_set.max_score());
    }

    return 0;
}


