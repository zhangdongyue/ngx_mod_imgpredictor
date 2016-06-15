#!/bin/sh

if [ $# -lt 2 ]
then
	echo "Usage:xxx <host_list file> <nginx_new>"
	exit 1
fi

while read line
do
	scp $2 $line:%%prefix%%/sbin/nginx.up
	#scp ngx_restart.sh  $line:%%prefix%%/sbin/
	

done < $1

#pssh -Ph imaginx-install.lst mv -f /home/admin/www/imaginx/sbin/nginx.up /home/admin/www/imaginx/sbin/nginx
#pssh -Ph imaginx-install.lst sh /home/admin/www/imaginx/sbin/ngx_restart.sh

