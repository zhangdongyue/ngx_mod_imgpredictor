
kill -USR2 `cat %%prefix%%/sbin/nginx.pid`

kill -QUIT `cat %%prefix%%/sbin/nginx.pid.oldbin`
