/*********************************************************
 * Copyright (c) 2015 DONGYUE.ZDY. All Rights Reserved
 * PluginSlot.h,v1.0.0,2016/01/19 09:59:00 
 *********************************************************/

/*
 *@author dongyue.zippy
 *@mailto zhangdy1986@gmail.com
 *@date 2016/01/18
 *@file PluginSlot.h
 *@brief 
 *
 *@update-1 (modicode:aextp) 
 *@author dongyue.zippy
 *@date 2016/04/13 
 *@note plugin处理函数原为接受2个参数，
 *   有些算法服务需要接收额外的参数，
 *   增加功能支持，
 *   额外的参数都封装在一个字符串内传入。
 *   plugin_info_t中增加扩展参数的判定项。 
 *   TODO:这里最好全部用json或者protobuf
 *   来定义，时间关系简单实现下。
 *
 * */

#ifndef __PLUGIN_SLOT_H_
#define __PLUGIN_SLOT_H_

#include<map>
#include<vector>
#include<utility>
#include<string>
#include<iostream>

extern "C"{
#include<dlfcn.h>
}

namespace gp {

struct plugin_info_t;
typedef std::map<std::string, plugin_info_t*> PsMap;
typedef std::map<std::string, plugin_info_t*>::const_iterator PsMapIterator;

typedef int 
    (*init_handler_t)(const std::string conf, const std::string model_dir);

/*
 * param[in] img_name_list: pair<image_local_path, image_url>
 * */
typedef int 
    (*process_handler_t)(const std::vector<std::pair<std::string,std::string > > img_name_list, std::string& res_json, ...);
    //old>> (*process_handler_t)(const std::string img_name, std::string& res_json, ...);

struct plugin_info_t {

	plugin_info_t():
		_dlhandler(NULL),
        _init_handler(NULL),
        _process_handler(NULL),
        _has_ext_in(false)
	{}

	~plugin_info_t(){
		if(_dlhandler)
			dlclose(_dlhandler);
	}


	std::string _name;
	void * _dlhandler; 

	init_handler_t 		_init_handler;
	process_handler_t 	_process_handler;


	std::string _conf;
	std::string _model_dir;

    /* Add(20160413-aextp-1) by dongyue.zdy@alibaba-inc.com 
     * when process_handler has more than two parameters,
     * another input parameters is in one string(_ext_input) 
     * by json or any method defined by user
     * */
    bool _has_ext_in;
    /*Add(20160413-aextp-1) end ~*/

};

class PluginSlot {
	public :
		PluginSlot() {}

		virtual ~PluginSlot(){

			if(!_pinfo_list.empty())
				_pinfo_list.clear();
		}

		bool load(const std::string ps_conf);

		/* 加载所有算法模型 0-succ 1-failed */
		int run_all_init(){
			if(_pinfo_list.empty())
				return 0;
			PsMapIterator psmIter = _pinfo_list.begin();
			for(;psmIter!=_pinfo_list.end(); ++psmIter) {
				plugin_info_t * pit = psmIter->second;
				if(pit->_init_handler(pit->_conf, pit->_model_dir) != 0)
				{
					std::cerr << "_init_handler process failed:plugin_name->"<<pit->_name
						<<"conf->"<<pit->_conf
						<<"model_dir="<<pit->_model_dir<<std::endl;

					return 1; 
				}
			}

			return 0;
		}

		/* 根据算法插件名称,执行算法.0-succ 1-fail */
		int run_process_by_name(const std::string plugin_name, 
								const std::vector<std::pair<std::string,std::string> > img_name_vec, 
                                const std::string ext_para,
								std::string& res_json); 

		plugin_info_t* get_plugin_info(const std::string plugin_name) const{
			PsMapIterator iter_pit;

			if(_pinfo_list.empty())
				return NULL;

			iter_pit = _pinfo_list.find(plugin_name);
			if(iter_pit == _pinfo_list.end())
				return NULL;

			return (plugin_info_t*)iter_pit->second;
		}

		bool insert_plugin(std::string name, plugin_info_t* pit){
			if(name.empty() || !pit)
				return false;

			_pinfo_list.insert(std::pair<std::string,plugin_info_t*>(name,pit));

			return true;
		}

		void erase_plugin(const std::string name){
			_pinfo_list.erase(name);
		}


	private:

		PsMap _pinfo_list; 


};//PluginSlot

}//namespace gp 

#endif
