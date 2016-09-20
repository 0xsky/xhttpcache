/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/

#ifndef    _X_UTIL_H_
#define    _X_UTIL_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <inttypes.h>
#include <zlib.h>
#include "xLog.h"


#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;

// is big endia. TODO: auto detect
#if 0
#define big_endian(v) (v)
#else
static inline uint16_t big_endian(uint16_t v){
    return (v >> 8) | (v << 8);
}

static inline uint32_t big_endian(uint32_t v){
    return (v >> 24) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | (v << 24);
}

static inline uint64_t big_endian(uint64_t v){
    uint32_t h = v >> 32;
    uint32_t l = v & 0xffffffffull;
    return big_endian(h) | ((uint64_t)big_endian(l) << 32);
}
#endif

#define encode_score(s) big_endian((uint64_t)(s))
#define decode_score(s) big_endian((uint64_t)(s))

uint64_t getCurrentTime();
const char *toGMT(time_t t);

void hash_dump(const char* str, int len);
char *hex_str(unsigned char* buf, int len, char *outstr);
std::string str(int64_t v);
int64_t str_to_int64(const std::string &str);
int str2Vect(const char* pSrc, vector<string> &vDest, const char *pSep=",");
int daemonize(void);
void guard(void(*driverFunc)(), int exit_key);
int base64_encode(const unsigned char *in, unsigned long inlen, unsigned char *out, unsigned long *outlen);
bool encode_pass(std::string &user, std::string pass, std::string &encodepass);
int gzcompress(Bytef *data, uLong ndata, Bytef *zdata, uLong *nzdata);


#endif

