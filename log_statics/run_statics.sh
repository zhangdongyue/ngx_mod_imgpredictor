#!/usr/bin/sh

logs_path="/home/admin/www/imaginx/logs/"

log_error="error.log"

/usr/bin/python /home/admin/www/imaginx/sbin/statics_imaginx_log.py ${logs_path}${log_error}_$(date +"%Y%m%d").log  600 1>>/dev/null 2>>${logs_path}statics_imaginx_log.log



