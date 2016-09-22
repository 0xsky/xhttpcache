
#What is xhttpcache? ([中文](README_中文.md))
Xhttpcache is a HTTP static cache service, which is also NOSQL database as K-V storage supporting REDIS protocol interface as well as REST interface of HTTP protocol.

#What are the functions of Xhttpcache? 

* Provision of caching services for HTTP data and support for binary data storage; 
* Realization of full K-V storage and permanent data storage (ROCKSDB);
* Support for data timeout mechanism with Millisecond accuracy;
* Implementation of the complete REDIS protocol interface with the SET/GET/DEL/EXIRE/TTL order for redis;
* Implementation of the HTTP/HTTPS protocol interface as well as access via REST command;
* Implementation of HTTP cache related protocol in support of eTage ,304 Not Modified and gzip compression for transmission content;
* Data Adjunction/ modification / deletion directly with WEB background editor 

###Graphic Xhttpcache
![xhttpcache](http://xhttpcache.0xsky.com/images/plans.png)

###Compilation and installation

Compiler dependency Library: Need compiling and installing in advance
```

```
build
```
wget --no-check-certificate https://github.com/0xsky/xhttpcache/archive/master.zip 
unzip master.zip 
cd xhttpcache-master 
make
```

##Approaches:
xhttpcache is designed as accelerated server for HTTP data: Data written to the xhttpcache by providing the write interface accessed directly through the browser. Meanwhile, reading and writing through redis` client(support a variety of languages) with the help of a redis protocol interface, which serves as a simple NOSQL databases; Established K-V data through a REDIS interface offering direct access via the HTTP interface in a browser 

The usage of NOSQL database: establish the K-V data through the REDIS interface, visit the database through the HTTP interface in the browser 

Access via HTTP interface
There are two HTTP service port in xhttpcache configured in the configuration file. Httpd_frontend_port is an open visiting port with read-only requests while Httpd_backend_port is the back-end data ports supporting all the REST reading and writing requests (get/post/put/delete); Back-end data access can be set logining password;

###The samples under the redis command:
The following example shows all the supported REDIS commands
```bash
    [xsky@localhost xhttpcache]$ redis-cli -p 7379
    127.0.0.1:7379> set test hello 
    OK
    127.0.0.1:7379> get test
    "hello"
    127.0.0.1:7379> set test hello ex 1000
    OK
    127.0.0.1:7379> get test
    "hello"
    127.0.0.1:7379> ttl test
    (integer) 988
    127.0.0.1:7379> EXPIRE test 2000
    (integer) 1
    127.0.0.1:7379> ttl test
    (integer) 1998
    127.0.0.1:7379> get test
    "hello"
    127.0.0.1:7379>
```
###Add image files to xhttpcache via the redis command
```bash
redis-cli -p 7379 -x set getheadimg.jpg <getheadimg.jpg
```

###Bulk of data imports and exports
Import bulk of disk file to the xhttpcache through the following script leading  the files in the directory specified by the parameter into the xhttpcache. 
```bash
#!/bin/bash

redis_cmd="redis-cli -p 7379 -x set "
function batch_convert() {
    for file in `ls $1`
    do
        if [ -d $1"/"$file ]
        then
            batch_convert $1"/"$file
        else
            key=$1"/"$file       
            #echo 33 $redis_cmd  ${key#*/}   $1"/"$file
            $redis_cmd ${key#*/} < $1"/"$file
        fi
    done
}

batch_convert $1
```
The data in the xhttpcache can be exported to the disk in the form of a directory file by the following shell script,
```bash
#!/bin/sh
# 
# xHttpCache数据备份脚本
# By: xSky

usage()
{
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++" 
    echo "+ xHttpCache data export shell tools by:xSky       " 
    echo "+ usage:                                           " 
    echo "+ $0 data_dir redis_host redis_port                " 
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++" 
}

#判断参数
if [ $# -ne 3 ]; then
    usage
    exit 0
fi

data_dir=$1
redis_host=$2
redis_port=$3
echo $data_dir $redis_host $redis_port

if [ -d $data_dir ];then
    #redis-cli -p 6579 scan a z | awk  '{print "redis-cli -p 6579 get "$0 " >"$0}'|sh
    cd $data_dir
    redis-cli -h $redis_host -p $redis_port scan a z | while read line
    do
        echo $line 
        bdir=`expr index $line "/"`
        file_dir=${line%/*}
        file_name=$line

        if [[ $bdir -gt 0 ]];then
            #echo 1 $bdir $file_dir $file_name
            mkdir -p $file_dir
        fi

        redis-cli -h $redis_host -p $redis_port get $line > $line

    done
    cd -
else
    echo [ERROR] the dir $data_dir do not exist
    exit 0
fi
```

###Characteristic description:
Xhttpcache can return the corresponding file suffix Content-Type judging the URI type visited by HTTP, which makes its HTTP interface a real WEB server. When accessed through the browser, the data will be displayed directly in the form of a page instead of the usual data interface.
For example: for the address with the /test.htm form. When returning contents, the HTTP head returns: text/html Content-Type, which can be displayed in the form of HTML through the browser directly to the test.htm corresponding data content, 
For the key in the /test.jpg form, if the value content should be JPG image data, the normal display as a picture appears through the browser to access the /test.jpg. Besides, the background supports the previewing and uploading of the pictures.
According to the above characteristics, it can be very easy to load all the static web site files to cache with full memory.
   
###Gratitude
Be grateful of the following items. Ranking regardless.
* [libevent](http://libevent.org/) 
* [libevhtp](https://github.com/ellzey/libevhtp)
* [rocksdb](http://rocksdb.org/)
* [ssdb](http://ssdb.io/)

###About the author
* [xhttpcache websit](http://xhttpcache.0xsky.com/)
* xhttpcache QQGroup: 195957781
* [xSky blog](http://www.0xsky.com/) 
* [Email](guozhw@gmail.com)


