/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/

#ifndef _xDATA_BASES_H_
#define _xDATA_BASES_H_

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/statistics.h"
#include "rocksdb/utilities/db_ttl.h"
#include "xConfig.h"
#include "xLog.h"
#include "sorted_set.h"
#include "util.h"
#include "xEtagManager.h"

using namespace std;
using namespace rocksdb;
   
//#define MAX_KEY_LEN 255

enum ret_code {
	db_ok = 0,
	db_err = 1,
	db_null = 2,
};

enum DataType{
    K_STRING = 0x01,
    K_ZSET   = 0x02,
    K_ZSCORE = 0x03,
};

class DataTypeFlag{
public:
    static const char KV     = 'k';
    static const char ZSET   = 's';
    static const char ZSCORE = 'z';
};

struct xValue
{
    uint64_t timestamp;
    std::string strValue;

    std::string &str() {
        return strValue;
    }
    void decode() {
        memcpy((char*)&timestamp, strValue.data(), sizeof(uint64_t));
        strValue.erase(0, sizeof(uint64_t));
    }
    void encode() {
        strValue.insert(0, (char*)&timestamp, sizeof(uint64_t));
    }
};

class xDataBase
{
public:
	xDataBase();
	virtual ~xDataBase();

    bool OpenDB(const ROCKSDB_CONFIG &cfg);
    void Close();

private:
    bool EncodeKey(std::string &buf, int tabtype, const std::string &key, 
        const std::string *filed = NULL, const std::string * score = NULL);
    bool DecodeKey(const std::string &rowkey, std::string *key,
        std::string *member = NULL, std::string *score = NULL);
public:
	void StatusStr(std::string &str); 
	int Set(const std::string &key, const std::string &val);
    int Get(const std::string &key, xValue &val);
	int Del(const std::string &key);
    int Rawdel(const std::string &key);
    int Expire(const std::string &key);
    int ScanKey(std::string &start, std::string &end, int limit, std::vector<std::string> &vkey);
    int Scan(std::string &start, std::string &end, int limit, std::vector<std::string> &vkey);
    int Flushall();
    void Debug();

    int Zset(const std::string &key, const std::string &member, const std::string &score);
    int Zget(const std::string &key, const std::string &member, std::string &score);
    int Zdel(const std::string &key, const std::string &member);
    int LoadExpirationKeys(const std::string &expirekey, SortedSet &keylist, int limit);

    void SetEtag(xEtagManager *etag, bool betag = false);
    bool CheckEtag(const std::string &key, uint64_t timestamp);
    xEtagManager *GetEtag();

private:
    DB* db;
    Options options;
    xEtagManager *xEtag;
    bool bEtag;
};

#endif

