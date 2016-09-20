#!/bin/bash

# 需要设置好实际的端口和redis-cli的路径
redis_cmd="redis-cli -p 7379 -x set "
function batch_convert() {
    for file in `ls $1`
    do
        if [ -d $1"/"$file ]
        then
            batch_convert $1"/"$file
        else
            #dos2unix $1"/"$file
            #if [ $1 = "."  ] 
            #then
            #    #echo 11 $redis_cmd $file   $1"/"$file
            #else
            #    #echo 22 $redis_cmd  $1"/"$file   $1"/"$file
            #fi
            key=$1"/"$file       
			#echo 33 $redis_cmd  ${key#*/}   $1"/"$file
            $redis_cmd ${key#*/} < $1"/"$file
        fi
    done
}

batch_convert $1