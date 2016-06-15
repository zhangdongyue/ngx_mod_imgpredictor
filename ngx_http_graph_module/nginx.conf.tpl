#
#daemon off;
#master_process off;
#user  dongyue.zdy;
worker_processes  8;
worker_cpu_affinity 00000001 00000010 00000100 00001000 00010000 00100000 01000000 10000000;

#error_log  logs/error.log	debug;
#error_log  logs/error.log  notice;
error_log  logs/error.log;
#error_log  logs/error.log  info;

pid        sbin/nginx.pid;


events {
	use epoll;
    worker_connections  65536;
	accept_mutex on;
}

thread_pool fpthp threads=16 max_queue=32;

http {
    include       mime.types;
    default_type  application/octet-stream;

    log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
                      '$status $body_bytes_sent "$http_referer" '
                      '"$http_user_agent" "$http_x_forwarded_for"';

    access_log  logs/access.log  main;

	#access_log  off;
    sendfile        on;
	sendfile_max_chunk 512k;
    #tcp_nopush     on;

    keepalive_timeout  0;
    #keepalive_timeout  65;

    #gzip  on;

    server {

        listen       8090;
        server_name  test.nginx;

        #charset koi8-r;

        #access_log  logs/host.access.log  main;

		location ~ /imgprocess$ {

			#client_body_in_file_only	off;
			#client_body_in_single_buffer on;
			error_log  logs/err_info.log;

			aio threads=fpthp;
			client_body_buffer_size		2M;
			fp %%prefix%%/conf/dl_lib_info.conf;
			root	html;
		}

		location / {
			root   html;
			index  index.html index.htm;
		}

#error_page  404              /404.html;

# redirect server error pages to the static page /50x.html
#
		error_page   500 502 503 504  /50x.html;
		location = /50x.html {
			root   html;
		}


# proxy the PHP scripts to Apache listening on 127.0.0.1:80
#
#location ~ \.php$ {
#    proxy_pass   http://127.0.0.1;
#}

# pass the PHP scripts to FastCGI server listening on 127.0.0.1:9000
#
#location ~ \.php$ {
#    root           html;
#    fastcgi_pass   127.0.0.1:9000;
#    fastcgi_index  index.php;
#    fastcgi_param  SCRIPT_FILENAME  /scripts$fastcgi_script_name;
#    include        fastcgi_params;
#}

# deny access to .htaccess files, if Apache's document root
# concurs with nginx's one
#
#location ~ /\.ht {
#    deny  all;
#}
	}


# another virtual host using mix of IP-, name-, and port-based configuration
#
#server {
#    listen       8000;
#    listen       somename:8080;
#    server_name  somename  alias  another.alias;

#    location / {
#        root   html;
#        index  index.html index.htm;
#    }
#}


# HTTPS server
#
#server {
#    listen       443 ssl;
#    server_name  localhost;

#    ssl_certificate      cert.pem;
#    ssl_certificate_key  cert.key;

#    ssl_session_cache    shared:SSL:1m;
#    ssl_session_timeout  5m;

#    ssl_ciphers  HIGH:!aNULL:!MD5;
#    ssl_prefer_server_ciphers  on;

#    location / {
#        root   html;
#        index  index.html index.htm;
#    }
#}

}
