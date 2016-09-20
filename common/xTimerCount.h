/*
* ----------------------------------------------------------------------------
* Copyright (c) 2015-2016, xSky <guozhw at gmail dot com>
* All rights reserved.
* Distributed under GPL license.
* ----------------------------------------------------------------------------
*/

#ifndef    _X_TIME_COUNT_H_
#define    _X_TIME_COUNT_H_


class xTimeCount {
    uint32_t _StartTime;
    uint32_t _CpuTime;
    const char* _str;
public:
    xTimeCount(const char* obj) :_StartTime(gettickcount(CLOCK_MONOTONIC)), _CpuTime(gettickcount(CLOCK_THREAD_CPUTIME_ID)), _str(obj){}
    ~xTimeCount(){
        uint32_t tm = gettickcount(CLOCK_MONOTONIC) - _StartTime;
        uint32_t cputm = gettickcount(CLOCK_THREAD_CPUTIME_ID) - _CpuTime;
        if (tm > 5 || cputm > 5){ log_warn("%s funcall time:%d cputime:%d ", _str, tm, cputm); }
    }

    inline unsigned long gettickcount(clockid_t id = CLOCK_MONOTONIC) {
        struct timespec ts;
        clock_gettime(id, &ts);
        return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    }

};

#define _TIME_TEST_(fun,line) xTimeCount fun##line(__PRETTY_FUNCTION__) //__LINE__
#define _TIME_TEST(fun,line) _TIME_TEST_(fun,line)
#define TIME_TEST _TIME_TEST(tm, __LINE__);


#endif


