/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/

#include "xDataBase.h"
#include "xLog.h"


xDataBase::xDataBase()
{
    db = NULL;
    bEtag = false;
    xEtag = NULL;
}

xDataBase::~xDataBase()
{

}

bool xDataBase::OpenDB(const ROCKSDB_CONFIG &cfg)
{
    options.IncreaseParallelism();
  	options.OptimizeLevelStyleCompaction();
    options.create_if_missing = cfg.create_if_missing;
    options.max_open_files = cfg.max_open_files;
    options.write_buffer_size = cfg.write_buffer_size * 1024 * 1024;
    options.db_log_dir = cfg.db_log_dir;

    if (cfg.compression) {
        options.compression = kSnappyCompression;
    } else {
        options.compression = kNoCompression;
    }

    Status status;
    status = DB::Open(options, cfg.db_base_dir, &db);
    if (!status.ok()) {
        log_error("open db failed: %s", status.ToString().c_str());
        return false;
    }
    log_info("open db %s success: %s", cfg.db_base_dir.c_str(), status.ToString().c_str());
    return true;
}

void xDataBase::Close()
{
	delete db;
	db = NULL;
}

void xDataBase::StatusStr(std::string &str)
{
    options.statistics = rocksdb::CreateDBStatistics();
    str = options.statistics->ToString();
}

bool xDataBase::EncodeKey(std::string &buf, int keytype, const std::string &key, 
    const std::string *filed, const std::string * score)
{
    buf = "";
    switch (keytype) {
    case K_STRING: {
		// k|key:string
        buf.append(1, DataTypeFlag::KV);
        buf.append(key.data(), key.size());
        break;
    }
    case K_ZSET: {
        // S|key_len|key|member_len|member:score
        buf.append(1, DataTypeFlag::ZSET);
        buf.append(1, (uint8_t)key.size());
        buf.append(key.data(), key.size());
        buf.append(1, (uint8_t)filed->size());
        buf.append(filed->data(), filed->size());
        break;
    }
    case K_ZSCORE: {
        // Z|key_len|key|score|member:""
        buf.append(1, DataTypeFlag::ZSCORE);
        buf.append(1, (uint8_t)key.size());
        buf.append(key.data(), key.size());

        //log_debug("score: %s", score->c_str());
        int64_t s = str_to_int64(*score);
        s = encode_score(s);
        //log_debug("s: %lld", s);
        buf.append((char *)&s, sizeof(int64_t));
        buf.append(1, ':');
        buf.append(filed->data(), filed->size());
        break;
    }
    default:{
        log_error("set error: %s", key.c_str());
        return false;
    }
    }
    //log_debug("encode_key: %s\r\n", buf.c_str());
    //hash_dump(buf.c_str(), buf.length());
    return true;
}

bool xDataBase::DecodeKey(const std::string &rowkey, std::string *key,
    std::string *member, std::string *score)
{
    Decoder decoder(rowkey.data(), rowkey.size());
    char keytype = rowkey.c_str()[0];
    switch (keytype) {
    case DataTypeFlag::KV: {
        if (decoder.skip(1) == -1){
            return false;
        }
        if (decoder.read_data(key) == -1){
            return false;
        }
        break;
    }
    case DataTypeFlag::ZSCORE: {
        if (decoder.skip(1) == -1){
            return false;
        }
        if (decoder.read_8_data(key) == -1){
            return false;
        }
        int64_t s;
        if (decoder.read_int64(&s) == -1){
            return false;
        } else {
            if (score != NULL){
                s = decode_score(s);
                score->assign(str(s));
            }
        }
        if (decoder.skip(1) == -1){
            return false;
        }
        if (decoder.read_data(member) == -1){
            return false;
        }
        break;
    }
    default:{
        log_error("decode_key error: %s", rowkey.c_str());
        return false;
    }
    }

    return true;
}

int xDataBase::Set(const std::string &key, const std::string &val)
{
    log_debug("key:%s  val_len:%d \r\n", key.c_str(), val.length());
    string strKey;
    EncodeKey(strKey, K_STRING, key, NULL);

    xValue xVal;
    xVal.timestamp = getCurrentTime();
    xVal.strValue = val;
    xVal.encode();
    
    Status s = db->Put(WriteOptions(), strKey, xVal.str());
	if (!s.ok()) {
        log_error("set error: %s", s.ToString().c_str());
        return db_err;
    }
    if (bEtag) {
        xEtag->SetKey(key.c_str(), xVal.timestamp);
        log_debug("xEtag:%p  \r\n", xEtag);
    }

    return db_ok;
}

int xDataBase::Get(const std::string &key, xValue &xval)
{
    //log_debug("key:%s  \r\n", key.c_str());
    string strKey;
    EncodeKey(strKey, K_STRING, key, NULL);
    Status s = db->Get(ReadOptions(), strKey, &xval.str());
	if (s.IsNotFound()) {
        log_warn("get error: %s", s.ToString().c_str());
        return db_null;
    }
    if (!s.ok()) {
        log_error("get error: %s", s.ToString().c_str());
        return db_err;
    }
    xval.decode();
    if (bEtag) {
        xEtag->SetKey(key.c_str(), xval.timestamp);
    }

    return db_ok;
}

int xDataBase::Del(const std::string &key)
{
    //log_debug("xDataBase del:%s", key.c_str());
    string strKey;
    EncodeKey(strKey, K_STRING, key, NULL);
    Status s = db->Delete(WriteOptions(), strKey);
    if (!s.ok()) {
        log_error("del error: %s", s.ToString().c_str());
        return db_err;
    }
    return db_ok;
}

int xDataBase::Rawdel(const std::string &key)
{
    //log_debug("xDataBase del:%s", key.c_str());
    Status s = db->Delete(WriteOptions(), key);
    if (!s.ok()) {
        log_error("del error: %s", s.ToString().c_str());
        return db_err;
    }
    return db_ok;
}

int xDataBase::Expire(const std::string &key)
{
    xValue val;
    int ret = Get(key, val);
    if (db_ok==ret) {
        return db_ok;
    }

    return db_err;
}

int xDataBase::ScanKey(std::string &start, std::string &end, int limit, std::vector<std::string> &vkey)
{
    //log_debug("start:%s end:%s limit:%d \r\n", start.c_str(), end.c_str(), limit);
    Iterator* it = db->NewIterator(ReadOptions());
    if (!it->status().ok()) {
        log_error("scan error: %s", it->status().ToString().c_str());
        return db_err;
    }
    std::string start_key;
    std::string end_key;
    EncodeKey(start_key, K_STRING, start, NULL);
    EncodeKey(end_key, K_STRING, end, NULL);

    std::string strkey;
    int n = 0;
    for (it->Seek(start_key); 
        it->Valid() && ((it->key().ToString() < end_key)&&(limit>n)); 
        it->Next()) {
        //log_debug("%s\r\n", it->key().ToString().c_str());
        //strkey = it->key().ToString();
        DecodeKey(it->key().ToString(), &strkey);
        vkey.push_back(strkey);
        n++;
    }
    return db_ok;
}

int xDataBase::Scan(std::string &start, std::string &end, int limit, std::vector<std::string> &vkey)
{
    //log_debug("start:%s end:%s limit:%d \r\n", start.c_str(), end.c_str(), limit);
    Iterator* it = db->NewIterator(ReadOptions());
    if (!it->status().ok()) {
        log_error("scan error: %s", it->status().ToString().c_str());
        return db_err;
    }

    int n = 0;
    for (it->Seek(start);
        it->Valid() && ((it->key().ToString() < end) && (limit > n));
        it->Next()) {
        //log_debug("%s\r\n", it->key().ToString().c_str());
        //log_debug("%s\r\n", it->key().ToString().c_str());
        //hash_dump(it->key().ToString().c_str(), it->key().ToString().length());
        vkey.push_back(it->key().ToString());
        n++;
    }
    return db_ok;
}

int xDataBase::LoadExpirationKeys(const std::string &expirekey, SortedSet &keylist, int limit)
{
    std::string strStart, startEnd;
    std::string score_start;
    std::string score_end;
    std::string memberkey = "";
    
    static const char *SCORE_MIN = "0";
    static const char *SCORE_MAX = "9223372036854775807";
    score_start = SCORE_MIN;
    score_end = SCORE_MAX;

    EncodeKey(strStart, K_ZSCORE, expirekey, &memberkey, &score_start);
    EncodeKey(startEnd, K_ZSCORE, expirekey, &memberkey, &score_end);

    Iterator* it = db->NewIterator(ReadOptions());
    if (!it->status().ok()) {
        log_error("scan error: %s", it->status().ToString().c_str());
        return db_err;
    }

    std::string member;
    std::string score;
    std::string strkey;
    int n = 0;
    for (it->Seek(strStart);
        it->Valid() && ((it->key().ToString() < startEnd) && (limit > n));
        it->Next()) {
        strkey = it->key().ToString();
        if (strkey.data()[0] != DataTypeFlag::ZSCORE){
            log_error("%s\r\n", it->key().ToString().c_str());
            return db_err;
        }

        if (DecodeKey(strkey, NULL, &member, &score) == -1){
            continue;
        }
        log_debug("decode_zscore_key member:%s score:%s \r\n", member.c_str(), score.c_str());
        log_debug("%s\r\n", it->key().ToString().c_str());
        
        keylist.add(member, str_to_int64(score));
        n++;
    }
    return db_ok;
}

void xDataBase::Debug()
{
    Iterator* it = db->NewIterator(ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        log_debug("debug key:%s\r\n", it->key().ToString().c_str());
        log_debug("debug value:%s\r\n", it->value().ToString().c_str());
        hash_dump(it->key().ToString().c_str(), it->key().ToString().length());
        hash_dump(it->value().ToString().c_str(), it->value().ToString().length());
        //db->Delete(WriteOptions(), it->key().ToString());
    }
    assert(it->status().ok());
    delete it;
}

int xDataBase::Flushall()
{
    log_debug("flushall \r\n");
    Iterator* it = db->NewIterator(ReadOptions());
    if (!it->status().ok()) {
        log_error("scan error: %s", it->status().ToString().c_str());
        return db_err;
    }

    std::string strkey;
    for (it->SeekToFirst();it->Valid();it->Next()) {
        strkey = it->key().ToString();
        Status s = db->Delete(WriteOptions(), strkey);
        if (!s.ok()) {
            log_error("flushall del error: %s", s.ToString().c_str());
        }
    }

    return db_ok;
}

int xDataBase::Zset(const std::string &key, const std::string &member, const std::string &score)
{
    if (key.empty() || member.empty()){
        log_error("empty key or member!");
        return db_err;
    }

    //log_debug("zset %s %s %s \r\n", key.c_str(), member.c_str(), score.c_str());

    std::string old_score;
    int ret = Zget(key, member, old_score);
    if (ret == db_null || old_score != score) {
       // 这里需要写
        std::string enZkey; 
        std::string enSkey;
        if (ret!=db_null) {
            // 存在老的KEY
            log_debug("member:%s old_score: %s\r\n", member.c_str(), old_score.c_str());
            EncodeKey(enSkey, K_ZSCORE, key, &member, &old_score);
            log_debug("delete enSkey: %s\r\n", enSkey.c_str());
            db->Delete(WriteOptions(), enSkey);
        }

        EncodeKey(enZkey, K_ZSET, key, &member);
        EncodeKey(enSkey, K_ZSCORE, key, &member, &score);
        log_debug("enZkey: %s\r\n", enZkey.c_str());
        log_debug("enSkey: %s\r\n", enSkey.c_str());
        Status s = db->Put(WriteOptions(), enSkey, "");
        if (!s.ok()) {
            log_error("Put error: %s", s.ToString().c_str());
            return db_err;
        }
        s = db->Put(WriteOptions(), enZkey, score);
        if (!s.ok()) {
            log_error("Put error: %s", s.ToString().c_str());
            return db_err;
        }
        return db_ok;
    }

    return db_err;
}

int xDataBase::Zget(const std::string &key, const std::string &member, std::string &score)
{
    std::string enkey;
    EncodeKey(enkey,K_ZSET, key, &member);
    Status s = db->Get(ReadOptions(), enkey, &score);
    if (s.IsNotFound()){
        return db_null;
    }
    if (!s.ok()){
        log_error("zget error: %s", s.ToString().c_str());
        return db_err;
    }
    return db_ok;
}

int xDataBase::Zdel(const std::string &key, const std::string &member)
{
    if (key.empty() || member.empty()){
        log_error("empty key or member!");
        return db_err;
    }

    std::string old_score;
    int ret = Zget(key, member, old_score);
    if (ret == db_null) {
        return db_err;
    }

    std::string enZkey;
    std::string enSkey;
    EncodeKey(enZkey, K_ZSET, key, &member);
    EncodeKey(enSkey, K_ZSCORE, key, &member, &old_score);
    db->Delete(WriteOptions(), enSkey);
    Status s = db->Delete(WriteOptions(), enZkey);
    if (!s.ok()) {
        log_error("del error: %s", s.ToString().c_str());
        return db_err;
    }

    return db_ok;
}

bool xDataBase::CheckEtag(const std::string &key, uint64_t timestamp)
{
    return xEtag->GetKey(key, timestamp);
}

void xDataBase::SetEtag(xEtagManager *etag, bool betag)
{
    xEtag = etag;
    bEtag = betag;
}

xEtagManager *xDataBase::GetEtag()
{
    return xEtag;
}


