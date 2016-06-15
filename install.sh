#!/bin/sh -e 
#---------------------------------------------
# Copyright (c) dongyue.zdy All Rights Reserved
# Date 2016-02-22
# Brief install imaginx
#---------------------------------------------

if [ $# -lt 2 ]
then
	echo "Usage:install.sh <prefix> <update/install>."
	exit 1
fi

if [ ! -d $1 ]
then
	mkdir -p $1
fi

CURDIR=`pwd`

PREFIX=${1/%\//}

if [ $2 == "install" ]
then
	cd img_algorithms
	make 
	cp libs/* ../libs/
	make clean
	cd ..
	
	mkdir -p ${PREFIX}/libs
	\cp -rf ./libs ${PREFIX}/
fi

sed -e "s|%%prefix%%|${PREFIX}|g" sbin/log_rotate.sh.tpl > sbin/log_rotate.sh 
sed -e "s|%%prefix%%|${PREFIX}|g" ngx_http_graph_module/config.tpl > ngx_http_graph_module/config

cd 3rdlibs
tar zxvf pcre.tar.gz
cd ..
tar zxvf nginx-1.9.15.tar.gz
cd nginx-1.9.15
./configure --prefix=${PREFIX} --with-threads --add-module=../ngx_http_graph_module --with-debug --with-http_realip_module --with-pcre=${CURDIR}/3rdlibs/pcre
if [ $2 == "update" ]
then
	make
else
	make && make install
fi
cd ..

cd pssh_scripts
sed -e "s|%%prefix%%|${PREFIX}|g" cron.lst.tpl > cron.lst
sed -e "s|%%prefix%%|${PREFIX}|g" ngx_restart.sh.tpl > ngx_restart.sh 
sed -e "s|%%prefix%%|${PREFIX}|g" scp_imaginx.sh.tpl > scp_imaginx.sh 
sed -e "s|%%prefix%%|${PREFIX}|g" upgrade_ngx.sh.tpl > upgrade_ngx.sh
if [ $2 == "install" ]
then
	cp ngx_restart.sh ${PREFIX}/sbin/
fi
cd ..

if [ $2 == "update" ]
then 
	sed -e "s|%%prefix%%|${PREFIX}|g" ngx_http_graph_module/dl_lib_info.conf.tpl > dl_lib_info.conf
	#mv dl_lib_info.conf ${PREFIX}/conf/
	echo "Update process succ. after run blow:" 
	echo "###################################################"
	echo "sh pssh_scripts/upgrade_ngx.sh" 
	echo "pssh -Ph imaginx-install.lst mv -f ${PREFIX}/sbin/nginx.up ${PREFIX}/sbin/nginx"
	echo "pssh -Ph imaginx-install.lst sh ${PREFIX}/sbin/ngx_restart.sh"
	echo "###################################################"
	exit 0
fi

mkdir -p conf
sed -e "s|%%prefix%%|${PREFIX}|g" ngx_http_graph_module/nginx.conf.tpl > conf/nginx.conf
sed -e "s|%%prefix%%|${PREFIX}|g" ngx_http_graph_module/dl_lib_info.conf.tpl > conf/dl_lib_info.conf

\cp -rf ./conf/* ${PREFIX}/conf/
\cp -rf sbin/log_rotate.sh ${PREFIX}/sbin/

rm -rf conf	
rm -rf 3rdlibs/pcre
rm -rf nginx-1.9.15
