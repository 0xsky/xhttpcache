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