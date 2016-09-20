/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef UTIL_SORTED_SET_H
#define UTIL_SORTED_SET_H

#include <inttypes.h>
#include <string>
#include <map>
#include <set>

class SortedSet
{
public:
	bool empty() const{
		return size() == 0;
	}
	int size() const;
	int add(const std::string &key, int64_t score);
	// 0: not found, 1: found and deleted
	int del(const std::string &key);
	// the first item is copied into key if SortedSet not empty
	int front(std::string *key, int64_t *score=NULL) const;
	int back(std::string *key, int64_t *score=NULL) const;
	int64_t max_score() const;
	int pop_front();
	int pop_back();
	
	/*
	class Iterator
	{
	public:
		bool next();
		const std::string& key();
		int64_t score();
	};
	
	Iterator begin();
	*/

private:
	struct Item
	{
		std::string key;
		int64_t score;
		
		bool operator<(const Item& b) const{
			return this->score < b.score
				|| (this->score == b.score && this->key < b.key);
		}
	};
	
	std::map<std::string, std::set<Item>::iterator> existed;
	std::set<Item> sorted_set;
};


class Decoder{
private:
    const char *p;
    int size;
    Decoder(){}
public:
    Decoder(const char *p, int size){
        this->p = p;
        this->size = size;
    }
    int skip(int n){
        if (size < n){
            return -1;
        }
        p += n;
        size -= n;
        return n;
    }
    int read_int64(int64_t *ret){
        if (size_t(size) < sizeof(int64_t)){
            return -1;
        }
        if (ret){
            *ret = *(int64_t *)p;
        }
        p += sizeof(int64_t);
        size -= sizeof(int64_t);
        return sizeof(int64_t);
    }
    int read_uint64(uint64_t *ret){
        if (size_t(size) < sizeof(uint64_t)){
            return -1;
        }
        if (ret){
            *ret = *(uint64_t *)p;
        }
        p += sizeof(uint64_t);
        size -= sizeof(uint64_t);
        return sizeof(uint64_t);
    }
    int read_data(std::string *ret){
        int n = size;
        if (ret){
            ret->assign(p, size);
        }
        p += size;
        size = 0;
        return n;
    }
    int read_8_data(std::string *ret = NULL){
        if (size < 1){
            return -1;
        }
        int len = (uint8_t)p[0];
        p += 1;
        size -= 1;
        if (size < len){
            return -1;
        }
        if (ret){
            ret->assign(p, len);
        }
        p += len;
        size -= len;
        return 1 + len;
    }
};

/*
TODO: HashedWheel
Each item is linked in two list, one is slot list, the other
one is total list.
*/
/*
template <class T>
class SortedList
{
public:
	void add(const T data, int64_t score);
	T front();
	void pop_front();

	class Item
	{
	public:
		int64_t score;
		Item *prev;
		Item *next;
		//Item *slot_prev;
		//Item *slot_next;
		T data;
	};
};
*/

#endif
