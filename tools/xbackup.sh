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
        #else 
            #echo 2 $bdir $file_dir $file_name
        fi
        
        redis-cli -h $redis_host -p $redis_port get $line > $line
    
    done
    cd -
else
    echo [ERROR] the dir $data_dir do not exist
    exit 0
fi


