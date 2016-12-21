
--[[
 * @brief: service_config.lua

 * @author:	  kun si
 * @date:	2016-12-21
--]]

local service_config = {}

service_config["db_sprj_config"] = {
	dbType = "MySQL",	--
	totalNum = 15, 		-- mysql最大连接数
	host = "127.0.0.1",	-- mysql数据库IP
	port = 3306,		-- mysql数据库端口
	database = "sprj",	-- mysql数据库
	user = "interface",	-- mysql数据库用户名
	max_packet_size = 1024 * 1024,
	password = "H3/I/BdJecGOn5uP3ygKk/n4cSZM2OzPvwD3phcyELs=", -- mysql数据库密码
}

service_config["redis_6479_config"] = {
	dbType = "Redis",	--
	totalNum = 15,		--
	host = "127.0.0.1",	--
	port = 6479,		--
	db = 0,            	--
	auth = "jM+x3GFfjj2fQm4x9mWUtGZejd+2S1jfgm8FIo58apU=",
}
return service_config