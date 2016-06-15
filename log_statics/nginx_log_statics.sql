create table `imaginx_log_statics` (
	`id` int(10) unsigned not NULL auto_increment,
	`plugin` varchar(128) NOT NULL COMMENT '算法标识',
	`server_addr` varchar(128) NOT NULL COMMENT '服务器ip',
	`push` int(10) unsigned NOT NULL default '0' COMMENT '推送图片请求数',
	`pull` int(10) unsigned NOT NULL default '0' COMMENT '拉取图片请求数',
	`detect` int(10) unsigned NOT NULL default '0' COMMENT '算法识别数量',
	`sum` int(10) unsigned NOT NULL default '0' COMMENT '总请求数',
	`err` int(10) unsigned NOT NULL default '0' COMMENT '请求失败数',
	`date_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT '统计时间',
	primary key (`id`),
	unique key `plugin_dt` (`plugin`, `date_time`)
) ENGINE=InnoDB default charset=utf8;


create table `imaginx_http_statics` (
	`id` int(10) unsigned not NULL auto_increment,
	`server_addr` varchar(128) NOT NULL COMMENT '服务器ip',
	`sum` int(10) unsigned NOT NULL default '0' COMMENT '总请求数',
	`err` int(10) unsigned NOT NULL default '0' COMMENT '请求失败数',
	`date_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT '统计时间',
	primary key (`id`),
	unique key `ser_dt` (`server_addr`, `date_time`)
) ENGINE=InnoDB default charset=utf8;


