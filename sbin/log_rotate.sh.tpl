#/bin/bash

logs_path="%%prefix%%/logs/"

#以前的日志文件。

log_error="error.log"   
log_access="access.log"   

pid_path="%%prefix%%/sbin/nginx.pid"

#mv ${logs_path}${log_error} ${logs_path}${log_error}_$(date --date="LAST WEEK" +"%Y%m%d").log
mv ${logs_path}${log_error} ${logs_path}${log_error}_$(date +"%Y%m%d").log

mv ${logs_path}${log_access} ${logs_path}${log_access}_$(date +"%Y%m%d").log

kill -USR1 `cat ${pid_path}`
