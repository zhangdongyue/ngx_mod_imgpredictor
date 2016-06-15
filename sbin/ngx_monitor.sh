#!/bin/bash

#=============================================
#Copyright (c) Dongyue.z All Rights Reserved.
#=============================================

# Author:Dongyue.Zippy
# Date: 2016-03-02 15:06:00
# Brief: Monitor Nginx Process

host=`hostname`
ip=`ifconfig bond0 | grep 'inet addr'|sed 's/.*addr://g'|sed 's/B.*//g'`

#监控nginx的连接数
http_req=`netstat -nat|grep -i '8090'|wc -l `
time_stamp=`date "+%Y/%m/%d %T"`
if [ ${http_req} -ge 400 ];
then
	echo "alert ==> ${host}@${ip}: http connection ${http_req} >= 400 @${time_stamp} "
else
	echo "${host}@${ip}: http connection ${http_req} @ ${time_stamp}"
fi

##监控nginx的进程
nginx_proc=`ps -C nginx --no-heading | wc -l `
time_stamp=`date "+%Y/%m/%d %T"`
if [ ${nginx_proc} -lt 17 ]
then
	echo "alert ==> ${host}@${ip}: nginx process ${nginx_proc} < 17 @${time_stamp} "
else
	echo "${host}@${ip}: nginx process ${nginx_proc} @ ${time_stamp}"
fi

#监控nginx所占用的内存总数
nginx_mem=`top -b -n1 | grep nginx |gawk '{sum += $6}; END {print int(sum/1024)}' `
time_stamp=`date "+%Y/%m/%d %T"`
if [ ${nginx_mem} -ge 50 ]
then
	echo "alert ==> ${host}@${ip}: nginx memory usage ${nginx_mem} GB >= 50 @${time_stamp} "
else
	echo "${host}@${ip}: nginx memory ${nginx_mem}GB @ ${time_stamp}"
fi
