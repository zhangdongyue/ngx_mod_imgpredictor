#!/bin/env python
# -*- coding:utf-8 -*-
# coding:utf-8

#==============================================
#Copyright (c) Alibaba-Inc. All Rights Reserved.
#==============================================

#----------------------------------------------
# @File:statics_imginx_log.py
# @Author:dongyue.zippy
# @Date:2016-03-08 10:25:00
# @Mailto:dongyue.zdy@alibaba-inc.com
# @Brief:imaginx server log statics 
#----------------------------------------------

import json
import os,sys
import re
import socket
import datetime,time

sys.path.append("%%prefix%%/sbin/mysql_lib_26")

#import MySQLdb
import _mysql

plugin_dict = {'socol':'parse_Histsvm_badimg',\
	       'face':'parse_cicf',\
           'imgquality':'parse_img_qua',\
           'poi_tutu':'parse_tutu',\
           'tuwen':'parse_tutu',\
           'poi_yagai':'parse_yagai',\
           'poi_filter':'parse_poi_filter'}

src_type = ['push', 'pull']

res={}

#=============json parser funcs===========

def imaginx_str_to_json(json_str):
    imaginx_json = json.loads(json_str)
    if not imaginx_json :
        return None

    if not imaginx_json.has_key('result'):
        return None

    return imaginx_json
        

def parse_Histsvm_badimg(res_json):
	hb_res = imaginx_str_to_json(res_json) 
	if not hb_res:
		return False

	for res in hb_res['result']:
		if res.has_key('image_check'):
			if res['image_check'] != '0':
				return True 

	return False 

def parse_cicf(res_json):
	res_json = res_json.replace(",]", "]")
	cc_res = imaginx_str_to_json(res_json)
	if not cc_res:
		return False

	for res in cc_res['result']:
		if res.has_key('x'):
			return True
	return False 

def parse_tutu(res_json):
    tt_res = imaginx_str_to_json(res_json)
    if not tt_res:
        return False

    res = tt_res['result']
    if res.has_key('match'):
        if int(res['match']) != 0:
            return True

    return False

def parse_yagai(res_json):
    tt_res = imaginx_str_to_json(res_json)
    if not tt_res:
        return False

    res = tt_res['result']
    if res.has_key('overlap'):
        if int(res['overlap']) != 0:
            return True

    return False

def parse_img_qua(res_json):
    tt_res = imaginx_str_to_json(res_json)
    if not tt_res:
        return False

    res = tt_res['result']
    if res.has_key('state'):
        if res['state'] == 'true':
            return True

    return False

def parse_poi_filter(res_json):
    pf_res = json.loads(res_json)
    if not pf_res :
        return False 

    if not pf_res.has_key('results'):
        return False 

    res = pf_res['results']
    if res.has_key('label'):
        if res['label'] != '0':
            return True

    return False

#=============json parser funcs end===========

def insert_db(db,res, http_sum, http_err):
	for (plugin,value) in res.items():
		pull 	= value['pull'] if value.has_key('pull') else 0
		push 	= value['push'] if value.has_key('push') else 0
		detect 	= value['detect']
		err 	= value['err']
		sum 	= value['sum']
		datetime = value['datetime']
		serv_addr = value['server_addr']
		#print plugin,serv_addr,push,pull,detect,sum,err,datetime	

		try:
			db.query("insert into imaginx_log_statics \
				(plugin,server_addr,push,pull,detect,sum,err,date_time) \
				values ('%s','%s','%s','%s','%s','%s','%s','%s')" % \
				(plugin,serv_addr,push,pull,detect,sum,err,datetime) )	
		except Exception, e:
			print "insert db failed:",e
			continue;

	try:
		db.query("insert into imaginx_http_statics \
			(server_addr,sum,err,date_time) \
			values ('%s','%s','%s','%s')" % \
			(serv_addr,http_sum,http_err,datetime) )
	except:
		print "catch exception"

	return True


def log_statics(log_file,step_seconds):
    global plugin_dict
    global res

	#db=MySQLdb.connect(host="dashboard.mysql.rds.tbsite.net",user="rmark",
    
    
    try:
        db=_mysql.connect(host="dashboard.mysql.rds.tbsite.net",user="rmark",\
                          passwd="mysqld", db="dashboard")
    except:
        print "connect to db failed."
    
    sum = 0
    err = 0
    succ = 0
    onetime = True 

    local_srv_addr= socket.gethostbyname(socket.gethostname())
    with open(log_file, "rb") as lf:
        for line in lf:
            #strs = line.split(', ')
            date_time=''
            res_json=''
            request=''

            line_match = re.match(r'^(.+), res_json:(.+), client:(.+), '+
                      'server:(.+), request:(.+), host:(.+)$', line)
            if line_match :
                if line_match.lastindex != 6 :
                    continue
                date_time += line_match.group(1)
                res_json += line_match.group(2)
                request += line_match.group(5) 
            else:
                continue

            date_time = ' '.join(date_time.split()[0:2])
            dt = datetime.datetime.strptime(date_time, "%Y/%m/%d %H:%M:%S")
			#dt = dt- datetime.timedelta(seconds=step_seconds)
			
            if onetime:
                start_dt = dt
                onetime=False

            if (dt-start_dt).seconds >= step_seconds:
                insert_db(db, res, sum, err)
                start_dt = dt
                res={}
                sum = 0
                err = 0
			
            sum += 1

            match = re.match(r'.*plugin=(\w+)&src=(\w+).*',request)
            if match:
                if match.lastindex != 2:
                    continue
                plugin = match.group(1)
                src = match.group(2)
                if not res.has_key(plugin):
                    res[plugin] = {}
                    res[plugin]['detect'] = 0
                    res[plugin]['datetime'] = start_dt.__str__()
                    res[plugin]['server_addr'] = local_srv_addr
                    res[plugin]['sum'] = 0
                    res[plugin]['err'] = 0
                if not res[plugin].has_key(src):
                    res[plugin][src] = 0

                res[plugin][src] += 1
                res[plugin]['sum'] += 1
				
            else:
                err+=1
                continue

            if src not in src_type:
                res[plugin]['err'] += 1
                err+=1
                continue

            if not plugin_dict.has_key(plugin):
                res[plugin]['err'] += 1
                err+=1
                continue
            parse_func = eval(plugin_dict[plugin])
            if parse_func(res_json):
                res[plugin]['detect'] += 1

    if len(res) > 0:
        insert_db(db, res, sum, err)

    db.close()

    """
	##mysql 
	db=_mysql.connect(host="dashboard.mysql.rds.tbsite.net",user="rmark",
			passwd="mysqld", db="dashboard")

	db.query(r"select * from nginx_test")

	r = db.use_result()	

	for row in r.fetch_row(maxrows=0):
		for col in row:
			print col

	"""
    """
	lfile = open(log_file, "rb")
	for line in lfile.readlines():
		print line
	"""


if __name__ == '__main__' :
	if(len(sys.argv) < 3):
		print "usage:xx <log_file> <step_seconds>"
		sys.exit()
	try:
		log_statics(sys.argv[1],int(sys.argv[2]));
	except Exception, e:
        	print "Failed:",e

