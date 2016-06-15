/*********************************************************
 * Copyright (c) 2015 DONGYUE.ZDY All Rights Reserved
 * PluginSlot.cpp,v1.0.0,2016/01/19 09:59:00 
 *********************************************************/

/*
 *@author dongyue.zippy (zhangdy1986@gmail.com)
 *@date 2016/01/19
 *@version 1.0.0
 *@file PluginSlot.cpp.
 *@brief manager the plugins of graph process.
 * 
 * @update1 (modicode:aextp) see:PluginSlot.h
 * @date 2016/04/13
 * @author dongyue.zippy
 * @note tosee:PluginSlot.h
 * */

#include "PluginSlot.h"
#include <fstream>
#include <vector>

namespace gp {

/* util method: split <str> by <pattern> */
static std::vector<std::string> split(std::string str,std::string pattern)
{
  std::string::size_type pos;
  std::vector<std::string> result;
  str+=pattern;
  int size=str.size();
 
  for(int i=0; i<size; i++) {
    pos=str.find(pattern,i);
    if((int)pos<size) {
      std::string s=str.substr(i,pos-i);
      result.push_back(s);
      i=pos+pattern.size()-1;
    }
  }
  return result;
}

static std::string url_decode(std::string str){
    std::string ret;
    char ch; 
    int i, ii, len = str.length();

    for (i=0; i < len; i++){
        if(str[i] != '%'){
            if(str[i] == '+')
                ret += ' ';
            else
                ret += str[i];
        }else{
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch; 
            i = i + 2;
        }   
    }   
    return ret;
}

/* parser config file,load plugin info to map list */
bool PluginSlot::load(const std::string conf)
{
	if (conf.empty()){
		std::cerr<<"Config file name is null."<<std::endl;
		return false;
	}

	std::ifstream file;

	file.open(conf.c_str());
	if (!file) {
		std::cerr<<"Open file:"<<conf<<" failed."<<std::endl;
		return false;
	}

	std::string line;
	while (!file.eof()) {
		file >> line;
		if (line.empty() || line.c_str()[0] == '#')
			continue;

		std::vector<std::string> vec_info = split(line, "|");
		if (vec_info.size() < 6) {
			std::cerr<<"Config line:\""<<line
				<<"\" is invalid.info not enough."<<std::endl;
			return false;
		}


		for (int i=0; i<(int)vec_info.size(); ++i) {
			if(vec_info[i].empty() && i!=4 && i!=5) {
					std::cerr << "The neccessary info is null:"<< std::endl;
					return false;
			}
		}

		plugin_info_t * pit = new plugin_info_t();
		if (!pit) {
			std::cerr<<"Alloc new plugin_info_t failed."<<std::endl;
			return false;
		}

		/* open dynamic lib */
		pit->_dlhandler = dlopen(vec_info[1].c_str(),RTLD_LAZY);
		if (!pit->_dlhandler) {
			std::cerr<<"Dlopen dynamic lib:\""<<vec_info[1]<<"\" failed:"
				<<dlerror()<<std::endl;
			delete pit;
			return false;
		}

		/* plugin name */
		pit->_name.assign(vec_info[0]);

		/* init function handler */
		pit->_init_handler = (init_handler_t)dlsym(pit->_dlhandler, vec_info[2].c_str());
		if (!pit->_init_handler) {
			std::cerr<<"cerrr:"<<dlerror()<<std::endl;
			delete pit;
			return false;
		}

		/* process function handler */
		pit->_process_handler = (process_handler_t)dlsym(pit->_dlhandler, vec_info[3].c_str());
		if (!pit->_process_handler) {
			std::cerr<<"cerrr:"<<dlerror()<<std::endl;
			delete pit;
			return false;
		}

		/* config name */
		if (!vec_info[4].empty()) {
			pit->_conf.assign(vec_info[4]);
		}

		/* model dir */
		if (!vec_info[5].empty()) {
			pit->_model_dir.assign(vec_info[5]);
		}

        /* process_handler ext parameter
         * add(20160413-aextp-3) by dongyue.zdy */
        if (vec_info.size() > 6) {
            if (!vec_info[6].compare("true")) 
                pit->_has_ext_in = true;
        }
        /* Add(20160413-aextp-3) end ~*/

		this->insert_plugin(pit->_name, pit);

	}

	if(file)
		file.close();
	return true;

} /* PluginSlot::load */

int PluginSlot::run_process_by_name(const std::string plugin_name, 
                        const std::vector<std::pair<std::string,std::string> > img_name_vec, 
                        const std::string ext_para,
                        std::string& res_json) {

    plugin_info_t * pit = this->get_plugin_info(plugin_name);
    if(pit == NULL){
        std::cerr << "Get plugin info by name failed."<<std::endl;
        return 1;
    }

    std::cout << "plugin_name:"<<pit->_name
        <<" conf:"<<pit->_conf
        <<" _model_dir"<<pit->_model_dir
        <<std::endl;

    /* Add(20160413-aextp-2) by dongyue.zdy*/
    if (pit->_has_ext_in) {
        if (ext_para.empty()) {
            std::cerr << "extension parameter is invalid!null." 
                << std::endl;
            return 1;
        }

        std::string de_ext_para = url_decode(ext_para);
        if (de_ext_para.empty()) {
            std::cerr << "decode ext_para failed" <<std::endl;
            return 1;
        }

        return pit->_process_handler(img_name_vec, res_json, de_ext_para.c_str());
    }
    /* Add(20160413-aextp-2) end ~*/

    return pit->_process_handler(img_name_vec, res_json); 

} /* run_process_by_name */


}//namespace gp
