
--[[
 * @brief: cluster_code.lua

 * @author:	  kun si
 * @date:	2016-12-22
--]]



local code={}
code[1] = "cluster_gateway"
code[2] = "cluster_login"
code[3] = "cluster_database"
code[4] = "cluster_room"
code[5] = "cluster_center"
code[6] = "cluster_hall"
code[7] = "cluster_game"

code["cluster_login"] = {SERVICE = ".login"}
code["cluster_hall"] = {SERVICE = ".hall"}
code["cluster_game"] = {SERVICE = ".game"}

return code