/*****************************************************
 * Copyright (c) DONGYUE.ZDY. All Rights Reserved.
 * **************************************************/

/*
 * @author zippy
 * @date 2015/12/28
 * @mailto zhangdy1986@gmail.com 
 * @brief 
 * 
 * */

#include<iostream>
#include<vector>
#include<string>
#include<utility>

#include"PluginSlot.h"

extern "C" {
#include<ngx_config.h>
#include<ngx_core.h>
#include<ngx_http.h>
#include<dlfcn.h>
#include<curl/curl.h>
#include<string.h>
}

static ngx_int_t ngx_http_graph_handler(ngx_http_request_t * r);
//static char * ngx_http_graph_merge_loc_conf(ngx_conf_t * cf, void * parent, void * child);
static void * ngx_http_graph_create_loc_conf(ngx_conf_t * cf);
char * ngx_http_graph(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static void ngx_http_graph_client_body_handler1(ngx_http_request_t* r);
static void ngx_http_graph_client_body_handler2(ngx_http_request_t* r);
static ngx_int_t ngx_http_graph_init(ngx_conf_t * cf);

typedef int (*graph_process_handler_t)(u_char* in, u_char* out, size_t out_size);
typedef int (*graph_init_handler_t)(u_char* ld_path);

typedef struct {

    ngx_str_t type;
    ngx_str_t ext;
    ngx_chain_t out_cl;
    ngx_chain_t in_cl;
    ngx_uint_t max_body_len;
    std::vector<ngx_temp_file_t*> * temp_file_vec;
    ngx_uint_t temp_file_cur_idx;
    gp::PluginSlot * plugin_slot;

} ngx_http_graph_loc_conf_t;

static ngx_command_t ngx_http_graph_commands[] = {

    {
        ngx_string("fp"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1, /* graph <model_path> <config> <output_dir>*/
        ngx_http_graph,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    }, 

    ngx_null_command
};

static ngx_http_module_t ngx_http_graph_module_ctx = {

    NULL,		/* preconfiguration */
    ngx_http_graph_init,		/* postconfiguration */

    NULL,		/* create main configuration */
    NULL,		/* init main configuration */

    NULL,		/* create server configuration */
    NULL,		/* merge server configuration */

    ngx_http_graph_create_loc_conf,		/* create location configuration */
    NULL, //ngx_http_graph_merge_loc_conf		/* merge location configuration */
};

ngx_module_t ngx_http_graph_module = {
    NGX_MODULE_V1,	/*0, 0, 0, 0, 0, 0, 1*/
    &ngx_http_graph_module_ctx,	/* module context */
    ngx_http_graph_commands,	/* module directives */
    NGX_HTTP_MODULE,	/* module type */
    NULL,	/* init master */
    NULL,	/* init module */
    NULL,	/* init process */
    NULL,	/* init thread */
    NULL,	/* exit thread */
    NULL,	/* exit process */
    NULL,	/* exit master */
    NGX_MODULE_V1_PADDING	/* 0, 0, 0, 0, 0, 0, 0, 0 */
}; 

//==============================functions===================================

char * ngx_http_graph(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{

    ngx_http_graph_loc_conf_t * glcf = (ngx_http_graph_loc_conf_t *)conf;
    ngx_http_core_loc_conf_t * clcf;

    clcf = (ngx_http_core_loc_conf_t*)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    if (clcf == NULL){
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "get module loc config failed.");
        return (char*)NGX_CONF_ERROR;
    }

    clcf->handler = ngx_http_graph_handler;

    ngx_str_t * value;
    value = (ngx_str_t *)cf->args->elts;
    if(cf->args->nelts < 2)
        return (char*)NGX_CONF_ERROR;

    glcf->plugin_slot = new gp::PluginSlot();
    if(!glcf->plugin_slot)
    {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "Create PluginSlot failed.");
        return (char*)NGX_CONF_ERROR;
    }

    glcf->temp_file_vec = new std::vector<ngx_temp_file_t *>();
    if (!glcf->temp_file_vec) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "Create temp_file_vec failed.");
        return (char *)NGX_CONF_ERROR;
    }

    std::string plugin_conf;
    plugin_conf.append((const char*)value[1].data,value[1].len);
    if(!glcf->plugin_slot->load(plugin_conf)){
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "Plugin slot load config failed:%s.",plugin_conf.c_str());
        return (char*)NGX_CONF_ERROR;
    }

    /* run all init handler */
    if(glcf->plugin_slot->run_all_init() !=0 ){
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "plugin slot run all init failed.");
        return (char*)NGX_CONF_ERROR;
    }

    return (char*)NGX_CONF_OK;

} /* ngx_http_graph */

static void * ngx_http_graph_create_loc_conf(ngx_conf_t * cf)
{
    ngx_http_graph_loc_conf_t * conf;

    conf = (ngx_http_graph_loc_conf_t*)ngx_pcalloc(cf->pool, sizeof(ngx_http_graph_loc_conf_t));
    if(conf == NULL){
        return NGX_CONF_ERROR;
    }

    conf->max_body_len = 1024*1024;//默认最大图片为1m

    conf->temp_file_cur_idx = 0;

    conf->plugin_slot = NULL;

    return conf;

} /* ngx_http_graph_create_loc_conf */

static ngx_int_t ngx_http_graph_handler(ngx_http_request_t * r)
{
    ngx_int_t rc;

    //ngx_http_graph_loc_conf_t * glcf;
    //glcf = (ngx_http_graph_loc_conf_t*)ngx_http_get_module_loc_conf(r, ngx_http_graph_module);

    if(r->headers_in.content_length_n == 0){
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "missing graph body");
        return NGX_HTTP_BAD_REQUEST;
    }

    ngx_str_t value;
    if(r->args.len){
        /*if(ngx_http_arg(r, (u_char *)"plugin", 6, &glcf->type) != NGX_OK)
          return NGX_HTTP_BAD_REQUEST; */

        if(ngx_http_arg(r,(u_char *)"src", 3, &value) != NGX_OK) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "missing src=pull/push arg");
            return NGX_HTTP_BAD_REQUEST; 
        }
    }

    /*
       if(r->headers_in.content_length_n > glcf->max_body_len)
       {
       ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
       "exceed the limit of graph max body length:%u",glcf->max_body_len);
       return NGX_HTTP_BAD_REQUEST;
       }*/

    if(ngx_strncmp(value.data,"pull",4)==0) {
        r->request_body_in_file_only = 0;
        r->request_body_in_persistent_file = 0;
        r->request_body_in_single_buf = 1;
        rc = ngx_http_read_client_request_body(r,ngx_http_graph_client_body_handler2);
    }else if(ngx_strncmp(value.data,"push",4)==0){
        r->request_body_in_file_only = 1;
        r->request_body_in_persistent_file = 1;
        r->request_body_in_single_buf = 0;
        rc = ngx_http_read_client_request_body(r,ngx_http_graph_client_body_handler1);
    }else{
        return NGX_HTTP_BAD_REQUEST;
    }

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {  
        return rc;  
    }  

    return NGX_DONE; 

} /* ngx_http_graph_handler */

static ngx_int_t ngx_http_graph_init(ngx_conf_t * cf)
{
    //预留
    return NGX_OK;

} /* ngx_http_graph_init */

static ngx_int_t ngx_http_graph_client_send(ngx_http_request_t * r)
{
    ngx_buf_t * b;
    ngx_int_t rc;

    ngx_http_graph_loc_conf_t * glcf;
    glcf = (ngx_http_graph_loc_conf_t*)ngx_http_get_module_loc_conf(r, ngx_http_graph_module);

    b = (ngx_buf_t *)ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL){
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                      "Failed to allocate response buffer.");

        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_type.len = sizeof("text/html")-1;
    r->headers_out.content_type.data = (u_char*)"text/html";
    r->headers_out.content_length_n = glcf->out_cl.buf->last - glcf->out_cl.buf->pos; 

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc; 
    } 

    return ngx_http_output_filter(r, &glcf->out_cl);

} /* ngx_http_graph_client_send */

static void ngx_http_graph_data_append(ngx_http_request_t *r, ngx_str_t * gd, ngx_buf_t * bf)
{
    ngx_int_t bf_len = bf->last - bf->pos;
    if((int)(bf_len+gd->len) > r->headers_in.content_length_n){
        bf_len = r->headers_in.content_length_n - gd->len;
    }

    ngx_uint_t cp_len = bf->last - bf->pos;
    ngx_memcpy(gd->data + gd->len, bf->pos, cp_len);	
    gd->len += cp_len;

    return;
} /* ngx_http_graph_data_append */

/* The callback handler for 'push' image, 
 * only one image supported on type 'push'*/
static void ngx_http_graph_client_body_handler1(ngx_http_request_t* r)
{
    ngx_http_graph_loc_conf_t * glcf;
    glcf = (ngx_http_graph_loc_conf_t*)ngx_http_get_module_loc_conf(r, ngx_http_graph_module);

    std::string res_json;

    if(r->request_body->temp_file || r->request_body_in_file_only)
    {

        std::vector<std::pair<std::string,std::string> > image_name_vec;
        std::string image_name;
        image_name.append( (const char *)r->request_body->temp_file->file.name.data, 
                          r->request_body->temp_file->file.name.len);
        image_name_vec.push_back(std::make_pair(image_name,"pushed_image"));

        std::string plugin_name;
        std::string ext_para;
        if(r->args.len){
            if(ngx_http_arg(r, (u_char *)"plugin", 6, &glcf->type) != NGX_OK) {
                ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
                return ;
            }

            if(ngx_http_arg(r, (u_char *)"ext", 3, &glcf->ext) == NGX_OK) {
                ext_para.append((const char*)glcf->ext.data, glcf->ext.len);
            }
        }

        plugin_name.append((const char *)glcf->type.data, glcf->type.len);

        if(glcf->plugin_slot->run_process_by_name(
                plugin_name, image_name_vec, ext_para, res_json) != 0)
        {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                          "run_process_by_name failed.Plugin:%s",
                          plugin_name.c_str());

            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
    }

    if(res_json.empty()){
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                  ", res_json:%s",res_json.c_str());


    ngx_buf_t * b;
    b = (ngx_buf_t *)ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL){
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                      "Failed to allocate response buffer.");

        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    b->pos = (u_char*)ngx_pcalloc(r->pool, res_json.size()+1);
    ngx_memcpy(b->pos, res_json.data(), res_json.size());
    b->last = b->pos + res_json.size();
    b->memory = 1;
    b->last_buf = 1;

    glcf->out_cl.buf = b;
    glcf->out_cl.next = NULL;

    ngx_http_graph_client_send(r);
    ngx_http_finalize_request(r, NGX_OK); 
    return ;

} /* ngx_http_graph_client_body_handler1 */

static int ngx_http_graph_curl_write_cb(void * ptr, size_t size, size_t nmemb, ngx_http_request_t * r)
{
    ngx_http_graph_loc_conf_t * glcf;
    glcf = (ngx_http_graph_loc_conf_t*)ngx_http_get_module_loc_conf(r, ngx_http_graph_module);

    int inlen = size*nmemb;

    ngx_chain_t cl;
    ngx_buf_t * b = (ngx_buf_t *)ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    b->pos = (u_char*) ptr;
    b->last = b->pos + inlen;
    b->memory = 1;
    b->last_buf = 1;
    cl.buf = b;
    cl.next = NULL;

    ngx_int_t n = ngx_write_chain_to_temp_file(
        glcf->temp_file_vec->at(glcf->temp_file_cur_idx), &cl);

    if(n == NGX_ERROR){
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
    }

    glcf->temp_file_vec->at(glcf->temp_file_cur_idx)->offset += n;

    return inlen;

} /* ngx_http_graph_curl_write_cb */

static ngx_int_t ngx_http_graph_curl_get(ngx_http_request_t * req, const char * url)
{
    if(url == NULL)
        return NGX_ERROR;

    CURLcode r = CURLE_GOT_NOTHING;
    CURL * curlhandle = NULL;

    r=curl_global_init(CURL_GLOBAL_NOTHING);
    if(CURLE_OK != r)
        return NGX_ERROR;

    curlhandle = curl_easy_init();
    if(NULL == curlhandle)
        return NGX_ERROR;

    curl_easy_setopt(curlhandle, CURLOPT_URL, url);
    curl_easy_setopt(curlhandle, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(curlhandle, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(curlhandle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curlhandle, CURLOPT_MAXREDIRS, 5);

    curl_easy_setopt(curlhandle, CURLOPT_WRITEFUNCTION, ngx_http_graph_curl_write_cb);
    curl_easy_setopt(curlhandle, CURLOPT_WRITEDATA, req);

    r = curl_easy_perform(curlhandle);

    curl_easy_cleanup(curlhandle);
    curl_global_cleanup();

    if(r == CURLE_OK)
        return NGX_OK;
    else
        return NGX_ERROR;

} /* ngx_http_graph_curl_get */

static ngx_int_t ngx_http_graph_create_temp_file(ngx_http_request_t * r)
{
    ngx_temp_file_t           *tf;

    ngx_http_core_loc_conf_t * clcf;

    clcf = (ngx_http_core_loc_conf_t*)ngx_http_get_module_loc_conf(r, ngx_http_core_module);
    if (clcf == NULL){
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "get module loc config failed.");
        return NGX_ERROR;
    }

    ngx_http_graph_loc_conf_t * glcf;
    glcf = (ngx_http_graph_loc_conf_t*)ngx_http_get_module_loc_conf(r, ngx_http_graph_module);

    tf = (ngx_temp_file_t*)ngx_pcalloc(r->pool, sizeof(ngx_temp_file_t));
    if (tf == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                      "ngx_pcalloc temp file failed.");
        return NGX_ERROR;
    }    

    tf->file.fd = NGX_INVALID_FILE;
    tf->file.log = r->connection->log;
    tf->path = clcf->client_body_temp_path;
    tf->pool = r->pool;
    tf->persistent = 1;


    glcf->temp_file_vec->push_back(tf);
    if (ngx_create_temp_file(&tf->file, tf->path, tf->pool,tf->persistent, tf->clean, tf->access) != NGX_OK)
    {    
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "create temp file failed");	
        return NGX_ERROR;
    }    

    return NGX_OK;

} /* ngx_http_graph_create_temp_file */

/* The callback for 'pull' images, 
 * muti images supported */
static void ngx_http_graph_client_body_handler2(ngx_http_request_t* r)
{
    ngx_chain_t * cl;

    ngx_http_graph_loc_conf_t * glcf;
    glcf = (ngx_http_graph_loc_conf_t*)ngx_http_get_module_loc_conf(r, ngx_http_graph_module);

    ngx_str_t * gd = (ngx_str_t *)ngx_pcalloc(r->pool, sizeof(ngx_str_t));

    ngx_http_request_body_t * rb = r->request_body;

    /* Read client body as buffering */
    if(rb != NULL){
        cl = rb->bufs;
        if(!cl->next && !cl->buf->in_file){
            gd->data = cl->buf->pos;
            gd->len  = cl->buf->last - cl->buf->pos;

        }else{
            gd->data = (u_char*)ngx_pcalloc(r->pool,r->headers_in.content_length_n+1);
            //ngx_memzero(gd->data,r->headers_in.content_length_n+1);
            gd->len = 0;
        }

        for(; cl; cl=cl->next) {
            if(cl->buf->memory){
                ngx_http_graph_data_append(r, gd, cl->buf);
            }else if(cl->buf->in_file){
                ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                              "in file than one chain.len=%d\n",gd->len);	
            }
        }
    }else{
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                      "Had no graph body.");	
        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST); 
        return;
    }

    //TODO: add loop for muti input pictures.
    u_char * gd_dbuf = gd->data;
    u_char * gd_dbuf_end = gd->data + gd->len -1;
    gd_dbuf[gd->len] = '\0';

    //gd->data[gd->len] = '\0';
    if(ngx_strncmp(gd_dbuf,"imgurl:",7)==0)
        gd_dbuf += 7;
    else{
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    std::vector<std::pair<std::string,std::string> > image_name_vec;

    /* reset */
    glcf->temp_file_vec->clear();
    glcf->temp_file_cur_idx = 0;

    while (gd_dbuf < gd_dbuf_end) {

        if(ngx_http_graph_create_temp_file(r) != NGX_OK) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                          "Failed to create temp file");

            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        u_char * img_url = gd_dbuf;
        gd_dbuf = (u_char*)strchr((const char*)img_url, '|');
        if (gd_dbuf) {
            *gd_dbuf = '\0';
            gd_dbuf ++ ;
        } else {
            gd_dbuf = gd_dbuf_end;
        }

        if(ngx_http_graph_curl_get(r,(const char *)img_url) != NGX_OK) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                          "Failed to curl get img_file:%s",img_url);

            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        /* run image plugin process */
        std::string image_name;
        image_name.append( 
            (const char *)glcf->temp_file_vec->at(glcf->temp_file_cur_idx)->file.name.data, 
            glcf->temp_file_vec->at(glcf->temp_file_cur_idx)->file.name.len);
        glcf->temp_file_cur_idx ++;

        image_name_vec.push_back(std::make_pair(image_name,(const char*)img_url));

    }
    //TODO: add loop end here ~~~


    std::string plugin_name;
    std::string ext_para;
    if(r->args.len){
        if(ngx_http_arg(r, (u_char *)"plugin", 6, &glcf->type) != NGX_OK) {
            ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
            return ;
        }

        if(ngx_http_arg(r, (u_char *)"ext", 3, &glcf->ext) == NGX_OK) {
            ext_para.append((const char*)glcf->ext.data, glcf->ext.len);
        }
    }
    plugin_name.append((const char *)glcf->type.data, glcf->type.len);

    std::string res_json;
    if(glcf->plugin_slot->run_process_by_name(
            plugin_name, image_name_vec, ext_para, res_json) != 0)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                      "run_process_by_name failed.Plugin:%s",
                      plugin_name.c_str());

        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    if(res_json.empty()){
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                  ", res_json:%s",res_json.c_str());

    ngx_buf_t * b;
    b = (ngx_buf_t *)ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL){
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                      "Failed to allocate response buffer.");

        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    b->pos = (u_char*)ngx_pcalloc(r->pool, res_json.size()+1);
    ngx_memcpy(b->pos, res_json.data(), res_json.size());
    b->last = b->pos + res_json.size();
    b->memory = 1;
    b->last_buf = 1;

    glcf->out_cl.buf = b;
    glcf->out_cl.next = NULL;

    ngx_http_graph_client_send(r);
    ngx_http_finalize_request(r, NGX_OK); 

    return ;

} /* ngx_http_graph_client_body_handler2 */

