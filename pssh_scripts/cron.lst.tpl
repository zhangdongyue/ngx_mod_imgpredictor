1 0 * * *  sh  %%prefix%%/sbin/log_rotate.sh
10 0 * * *  sh  %%prefix%%/sbin/run_statics.sh
20 0 * * *  find %%prefix%%/logs -mtime +30 -exec rm -rf {} \;
30 0 * * *  find %%prefix%%/client_body_temp -mtime +2 -exec rm -rf {} \;
