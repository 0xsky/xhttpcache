
##xhttpcache是什么？
xhttpcache 是一个HTTP静态缓存服务，也可以做为K-V存储的NOSQL数据库

支持redis协议接口,支持HTTP协议的REST接口;

##xhttpcache有哪些功能？
* 提供HTTP数据缓存服务，支持存储二进制数据; 
* 完整的K-V存储实现,  支持数据持久化存储(ROCKSDB); 
* 支持数据超时机制，提供毫秒级精度。
* 现实完整REDIS协议接口，支持redis的SET/GET/DEL/EXIRE/TTL命令;
* 实现HTTP/HTTPS协议接口，支持通过REST命令访问;
* 实现HTTP cache相关协议，支持eTage，支持304 Not Modified, 支持对传输内容gzip压缩;
* 自带WEB后台编辑器，可以直接添加(上传)/修改/删除数据;
    
###xhttpcache图解
![xhttpcache](http://xhttpcache.0xsky.com/images/plans.png)


###编译安装
编译依赖库：
需要先编译安装好:
  [rocksdb](https://github.com/facebook/rocksdb)
  [libevhtp](https://github.com/ellzey/libevhtp)
  [libsnappy](https://github.com/google/snappy)
  [libjemalloc](http://jemalloc.net/)
  
```bash
wget --no-check-certificate https://github.com/0xsky/xhttpcache/archive/master.zip
unzip master
cd xhttpcache-master
make
``` 

##使用方法：
xhttpcache被设计为HTTP数据加速服务器:通过提供的写接口向xhttpcache写入的数据，
可以直接通过浏览器访问.
同时也提供了redis的协议接口，可以直接通过redis的client(支持各种语言接口)进行读写，
使之也可以当做简单的NOSQL数据库使用;
通过REDIS接口建立的K-V数据，可以直接通过HTTP接口在浏览器里访问查看
    
####通过HTTP接口访问：
xhttpcache 有两个HTTP服务端口，可以配置文件里配置;
httpd_frontend_port 为开放访问端口，只支持读请求;
httpd_backend_port  为后端数据操作端口，支持全部REST读写请求(get/post/put/delete);
后端数据接口访问可以设置登陆账号密码;

    curl -d "testdata" http://admin:admin123@127.0.0.1:9090/testkey
    通过POST请求向 xhttpcache 提交建立一条kv格式的数据：testkey -- testdata
    
    curl http://127.0.0.1:8080/testkey
    通过get请求访问testkey的数据
    通过前端端口访问，不需要密码验证，当然也可以通过后端接口带上密码访问：
    curl http://admin:admin123@127.0.0.1:9090/testkey
    这两种方式通过GET请求访问的数据是完全一样的。
    
    curl -I -X DELETE http://admin:admin123@127.0.0.1:9090/testkey
    通过http协议的delete请求删除testkey
    
    通过以下命令可以上传本地文件到 xhttpcache
    curl -F file=@/tmp/me.txt http://admin:admin123@127.0.0.1:9090/test.jpg
    
####通过redis命令使用示例：

以下示例显示了所有的支持的REDIS命令
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

通过redis命令添加图片文件到 xhttpcache:
```bash
redis-cli -p 7379 -x set getheadimg.jpg <getheadimg.jpg
```

###数据批量导入与导出

通过以下脚本向 xhttpcache 里批量导入磁盘文件,
以下脚本会将参数指定的目录下的文件都导入到 xhttpcache 里.

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

通过以下shell脚本，可以把 xhttpcache 里的数据按目录文件的形式导出到磁盘。
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

    
##特性说明：
    xhttpcache对通过HTTP访问的URI会进行类型判断，返回对应文件后缀的Content-Type，
    使得xhttpcache的HTTP接口像一个真正的WEB服务器,
    通过浏览器访问时，数据会直接以页面的形式显示。而不是像通常的数据接口一样。
    例如：对于/test.htm 形式的地址，在返回内容时，http头部里返回: Content-Type: text/html;
    这样，通过浏览器直接访问 test.htm 对应该的数据内容时，可以以HTML的形式显示。
    对于/test.jpg 形式的key，如果对应该的value内容是JPG图片数据，通过浏览器访问/test.jpg时，就直接正常显示为图片;
    并且后台还支持对图片的预览与上传;
    根据以上特点，可以很容易的将静态网站文件全部加载到cache里,全部内存化.

##感激
感谢以下的项目,排名不分先后

* [libevent](http://libevent.org/) 
* [libevhtp](https://github.com/ellzey/libevhtp)
* [rocksdb](http://rocksdb.org/)
* [ssdb](http://ssdb.io/)


##关于作者
* [xhttpcache 主页](http://xhttpcache.0xsky.com/)
* [xSky blog](http://www.0xsky.com/) 
* xhttpcache QQ群: 195957781
