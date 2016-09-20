/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/

#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include "util.h"
#include "xLog.h"


char *hex_str(unsigned char* buf, int len, char *outstr) 
{
    const char *set = "0123456789abcdef";
    if (NULL == buf){
        return outstr;
    }
    char *temp;
    unsigned char *end;
    len = (len > 1024) ? 1024 : len;
    end = buf + len;
    temp = &outstr[0];
    while (buf < end){
        *temp++ = set[(*buf) >> 4];
        *temp++ = set[(*buf) & 0xF];
        *temp++ = ' ';
        buf++;
    }
    *temp = '\0';
    return outstr;
}

void hash_dump(const char* str, int len) 
{
    char szTemp[1024 + 1] = { 0 };
    log_debug("hash_dump %s\n", hex_str((unsigned char*)str, len, szTemp));
}

uint64_t getCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t ms = tv.tv_sec;
    return ms * 1000 + tv.tv_usec / 1000;
}

const char *toGMT(time_t t)
{
    struct tm *gmt;
    gmt = gmtime(&t);
    char *strtime = asctime(gmt);
    strtime[strlen(strtime) - 1] = 0;
    return strtime;
}

// 这个函数可以去掉
std::string str(int64_t v){
    char buf[21] = { 0 };
    snprintf(buf, sizeof(buf), "%" PRId64 "", v);
    return std::string(buf);
}

int64_t str_to_int64(const std::string &str){
    const char *start = str.c_str();
    char *end;
    int64_t ret = (int64_t)strtoll(start, &end, 10);
    // the WHOLE string must be string represented integer
    if (*end == '\0' && size_t(end - start) == str.size()){
        errno = 0;
    } else {
        // strtoxx do not set errno all the time!
        if (errno == 0){
            errno = EINVAL;
        }
    }
    return ret;
}

int str2Vect(const char* pSrc, vector<string> &vDest, const char *pSep)
{  
   if(NULL == pSrc) {
       return -1;
   }
   
   int iLen= strlen(pSrc);
   if(iLen==0) {
       return -1;
   }

   char *pTmp= new char[iLen + 1];
   if(pTmp==NULL) {
       return -1;
   }

   memcpy(pTmp, pSrc, iLen);
   pTmp[iLen] ='\0';

   char *pCurr = strtok(pTmp,pSep);
   while(pCurr) {
       vDest.push_back(pCurr);
       pCurr = strtok(NULL, pSep);
   }

   delete[] pTmp; 
   return 0;
}

/**
base64 Encode a buffer (NUL terminated)
@param in      The input buffer to encode
@param inlen   The length of the input buffer
@param out     [out] The destination of the base64 encoded data
@param outlen  [in/out] The max size and resulting size
@return 0 if successful
*/
int base64_encode(const unsigned char *in, unsigned long inlen, unsigned char *out, unsigned long *outlen)
{
    static const char *codes = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    unsigned long i, len2, leven;
    unsigned char *p;

    assert(in != NULL);
    assert(out != NULL);
    assert(outlen != NULL);

    /* valid output size ? */
    len2 = 4 * ((inlen + 2) / 3);
    if (*outlen < len2 + 1) {
        *outlen = len2 + 1;
        return -1;
    }
    p = out;
    leven = 3 * (inlen / 3);
    for (i = 0; i < leven; i += 3) {
        *p++ = codes[(in[0] >> 2) & 0x3F];
        *p++ = codes[(((in[0] & 3) << 4) + (in[1] >> 4)) & 0x3F];
        *p++ = codes[(((in[1] & 0xf) << 2) + (in[2] >> 6)) & 0x3F];
        *p++ = codes[in[2] & 0x3F];
        in += 3;
    }
    /* Pad it if necessary...  */
    if (i < inlen) {
        unsigned a = in[0];
        unsigned b = (i + 1 < inlen) ? in[1] : 0;

        *p++ = codes[(a >> 2) & 0x3F];
        *p++ = codes[(((a & 3) << 4) + (b >> 4)) & 0x3F];
        *p++ = (i + 1 < inlen) ? codes[(((b & 0xf) << 2)) & 0x3F] : '=';
        *p++ = '=';
    }

    /* append a NULL byte */
    *p = '\0';

    /* return ok */
    *outlen = p - out;
    return 0;
}

bool encode_pass(std::string &user, std::string pass, std::string &encodepass)
{
    std::string strKey;
    strKey = user;
    strKey += ":";
    strKey += pass;

    unsigned char user_key[128] = { 0 };
    unsigned long  in_len = strKey.length();
    unsigned long  out_len = sizeof(user_key);
    base64_encode((unsigned char*)strKey.c_str(), in_len, user_key, &out_len);
    encodepass = (char*)user_key;
    return true;
}

int daemonize(void)
{
#ifndef WIN32
    int status;
    pid_t pid, sid;
    int fd;

    char appdir[1024];
    char* appdirptr = getcwd(appdir, 1024);

    pid = fork();
    switch (pid) {
    case -1:
        log_error("fork() failed: %s", strerror(errno));
        return 1;

    case 0:
        break;

    default:
        exit(0);
    }

    sid = setsid();
    if (sid < 0) {
        log_error("setsid() failed: %s", strerror(errno));
        return 1;
    }

    if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
        log_error("signal(SIGHUP, SIG_IGN) failed: %s", strerror(errno));
        return 1;
    }

    pid = fork();
    switch (pid) {
    case -1:
        log_error("fork() failed: %s", strerror(errno));
        return 1;

    case 0:
        break;

    default:
        exit(0);
    }

    umask(0);

    fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        log_error("open(\"/dev/null\") failed: %s", strerror(errno));
        return 1;
    }

    status = dup2(fd, STDIN_FILENO);
    if (status < 0) {
        log_error("dup2(%d, STDIN) failed: %s", fd, strerror(errno));
        close(fd);
        return 1;
    }

    status = dup2(fd, STDOUT_FILENO);
    if (status < 0) {
        log_error("dup2(%d, STDOUT) failed: %s", fd, strerror(errno));
        close(fd);
        return 1;
    }

    status = dup2(fd, STDERR_FILENO);
    if (status < 0) {
        log_error("dup2(%d, STDERR) failed: %s", fd, strerror(errno));
        close(fd);
        return 1;
    }

    if (fd > STDERR_FILENO) {
        status = close(fd);
        if (status < 0) {
            log_error("close(%d) failed: %s", fd, strerror(errno));
            return 1;
        }
    }

    if (chdir(appdirptr) != -1) {
        log_error("chdir() failed");
    }
#endif
    return 0;
}

void guard(void(*driverFunc)(), int exit_key)
{
#ifdef WIN32
    (void)exit_key;
    driverFunc();
#else
    do {
        pid_t pid = fork();
        if (0 == pid) {
            driverFunc();
            exit(0);
        }
        else if (pid > 0) {
            int status;
            pid_t pidWait = waitpid(pid, &status, 0);
            if (pidWait < 0) {
                log_error("waitpid() failed");
                exit(0);
            }

            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == exit_key) {
                    exit(0);
                }
            }
            log_error("Restart the program...", status);
        }
        else {
            log_error("fork() failed");
            exit(1);
        }
    } while (true);
#endif
}

/* Compress gzip data */
/* data 原数据 ndata 原数据长度 zdata 压缩后数据 nzdata 压缩后长度 */
int gzcompress(Bytef *data, uLong ndata, Bytef *zdata, uLong *nzdata)
{
    z_stream c_stream;
    int err = 0;

    if (data && ndata > 0) {
        c_stream.zalloc = NULL;
        c_stream.zfree = NULL;
        c_stream.opaque = NULL;
        //只有设置为MAX_WBITS + 16才能在在压缩文本中带header和trailer
        if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
            MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) return -1;
        c_stream.next_in = data;
        c_stream.avail_in = ndata;
        c_stream.next_out = zdata;
        c_stream.avail_out = *nzdata;
        while (c_stream.avail_in != 0 && c_stream.total_out < *nzdata) {
            if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK) return -1;
        }
        if (c_stream.avail_in != 0) return c_stream.avail_in;
        for (;;) {
            if ((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END) break;
            if (err != Z_OK) return -1;
        }
        if (deflateEnd(&c_stream) != Z_OK) return -1;
        *nzdata = c_stream.total_out;
        return 0;
    }

    return -1;
}


