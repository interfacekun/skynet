local skynet = require "skynet"
local netpack = require "netpack"
--local csoeckt = require "socket"
local socketdriver = require "socketdriver"
require "skynet.manager"
local sproto = require "sproto"
local sprotoloader = require "sprotoloader"



local client_number = 0
local nodelay = false
local connection = {}
local socket
local queue	
local MSG = {}
local CMD = {}
local room_number
local address
local port
local host
local send_request




function CMD.start(source, conf)
	assert(not socket)
	address = conf.address or "0.0.0.0"
	port = assert(conf.port)
	maxclient = conf.maxclient or 1024
	nodelay = conf.nodelay
	room_number = conf.number
	socket = socketdriver.listen(address, port)
	socketdriver.start(socket)
	skynet.error(string.format("Room[%d] listen on %s:%d", room_number, address, port))
	--LOG_INFO("Room[%d] listen on %s:%d", room_number, address, port)
	local service_name = "room_" .. room_number
	skynet.register(service_name)
	skynet.error("room service name is %s", service_name)
	return "start"
end

function CMD.getRoomAddress()
	return address
end
function CMD.getRoomPort()
	return port
end

local function send_package(fd, pack)
	local package = string.pack(">s2", pack)
	csocket.write(fd, package)
end

local function openclient(fd)
	if connection[fd] then
		socketdriver.start(fd)
		client_number = client_number + 1
		skynet.error("Client[%d] come in", fd)
		--LOG_INFO("Client come in")
		--send_package(fd,send_request("s2cinfo", 
		--{info = string.format("Welcome to game room[%d]", room_number)}))
	end
end

local function close_fd(fd)
	local c = connection[fd]
	if c ~= nil then
		connection[fd] = nil
		client_number = client_number - 1
	end
end

function MSG.open(fd, msg)
	skynet.error("Client[%d] ask open socket", fd)
	if client_number >= maxclient then
		socketdriver.close(fd)
		return
	end
	if nodelay then
		socketdriver.nodelay(fd)
	end

	connection[fd] = true

	openclient(fd)

end
function MSG.close(fd)
	if fd ~= socket then
		
		close_fd(fd)

		--LOG_INFO("Client disconnect")
		skynet.error("Client disconnect")
	else
		socket = nil
	end
end

function MSG.error(fd, msg)
	if fd == socket then
		socketdriver.close(fd)
		skynet.error(msg)
	else
		
		close_fd(fd)
	end
end

function MSG.warning(fd, size)
	if handler.warning then
		handler.warning(fd, size)
	end
end




local function dispatch_msg(fd, msg, sz)
	if connection[fd] then
		--handler.message(fd, msg, sz)
	else
		skynet.error(string.format("Drop message from fd (%d) : %s", fd, netpack.tostring(msg,sz)))
	end
end


MSG.data = dispatch_msg

local function dispatch_queue()
	local fd, msg, sz = netpack.pop(queue)
	if fd then	
		skynet.fork(dispatch_queue)
		dispatch_msg(fd, msg, sz)

		for fd, msg, sz in netpack.pop, queue do
			dispatch_msg(fd, msg, sz)
		end
	end
end

MSG.more = dispatch_queue


skynet.register_protocol {
	name = "socket",
	id = skynet.PTYPE_SOCKET,	-- PTYPE_SOCKET = 6
	unpack = function ( msg, sz )
		return netpack.filter( queue, msg, sz)
	end,
	dispatch = function (_, _, q, type, ...)
		queue = q
		if type then
			MSG[type](...)
		end
	end
}
host = sprotoloader.load(1):host "package"
send_request = host:attach(sprotoloader.load(2))


skynet.start(function ()
	skynet.dispatch("lua",function(session, source, cmd, ...)
		local  f = assert(CMD[cmd], cmd .. "not found")
		skynet.retpack(f(source, ...))
	end)
end)