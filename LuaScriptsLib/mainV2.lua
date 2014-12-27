--[[

This is currently a second version of main.lua 

It is a attempt to merge lunaworld.lua and lunadll.lua in one state.

]]


--Constants used by Lunadll. Do not modify them, or Lunadll with crash. 
-- ==DANGER ZONE START==
__lapiInit = "__onInit"
__lapiEventMgr = "eventManager"
-- ==DANGER ZONE END==
-- Global vars
__loadedAPIs = {}

__lunaworld = {} --lunaworld.lua
__lunaworld.__init = false
__lunaworld.loadAPI = function(api)
	return loadLocalAPI("lunaworld", api)
end
__lunalocal = {} --lunadll.lua
__lunalocal.__init = false
__lunalocal.loadAPI = function(api)
	return loadLocalAPI("lunadll", api)
end

__episodePath = ""
__customFolderPath = ""

-- ERR HANDLING v2.0, Let's get some more good ol' data
function __xpcall (f, ...)
  return xpcall(function () return f(unpack(arg)) end,
    function (msg)
      -- build the error message
      return "==> "..msg..'\n'.."============="..'\n'..debug.traceback()
    end)
end
-- ERR HANDLING END

function isAPILoaded(api)
	if(type(api)=="table")then
		for k,v in pairs(__loadedAPIs) do
			if(v == api)then
				return true
			end
		end
		
		if(__lunaworld.__init)then
			for k,v in pairs(__lunaworld.__loadedAPIs) do
				if(v == api)then
					return true
				end
			end
		end
		
		if(__lunalocal.__init)then
			for k,v in pairs(__lunalocal.__loadedAPIs) do
				if(v == api)then
					return true
				end
			end
		end
	elseif(type(api)=="string")then
		if(__loadedAPIs[api])then
			return true
		end
		
		if(__lunaworld.__init)then
			if(__lunaworld.__loadedAPIs[api])then
				return true
			end
		end
		
		if(__lunalocal.__init)then
			if(__lunalocal.__loadedAPIs[api])then
				return true
			end
		end
	end
	
	return false
end

local function doAPI(apiName)
	local searchInPath = {
		__episodePath,
		__customFolderPath,
		getSMBXPath().."\\LuaScriptsLib\\"}
	local func, err
	for _,v in pairs(searchInPath) do
		func, err = loadfile(v..apiName..".lua")
		if(func)then
			return func()
		end
		if(not err:find("such file"))then
			error(err,2)
		end
	end
	error("No API found \""..apiName.."\"",2)
end

local function loadCodeFile(tableAddr, path, preDefinedEnv)
	tableAddr.__loadedAPIs = {}
	
	local tEnv = preDefinedEnv or {}
	setmetatable( tEnv, { __index = _G } )
	
	local codeFile, err = loadfile(path)
	if codeFile then
		tableAddr.__init = true
		setfenv( codeFile, tEnv )
		codeFile()
	else
		if(not err:find("such file"))then
			windowDebug("Error: "..err)
		end
		return false
	end
	
	for k,v in pairs( tEnv ) do
		tableAddr[k] =  v
	end
	
	return true
end

--Preloading function.
--This code segment won't post any errors!
function __onInit(lvlPath, lvlName)
	
	--Load default libs
	
	local isLunaworld = true
	local isLunadll = true
	
	local status = {pcall(function() --Safe code: This code segment can post errors
		__episodePath = lvlPath
		__customFolderPath = lvlPath..string.sub(lvlName, 0, -5).."\\"
		local doLunaworld = true
		local doLunadll = true
		
		loadSharedAPI("uservar")
		local localLuaFile = nil
		local glLuaFile = lvlPath .. "lunaworld.lua"
		if(lvlName:find("."))then
			localLuaFile = lvlPath..string.sub(lvlName, 0, -5).."\\lunadll.lua"
		end	

		if(not loadCodeFile(__lunaworld, glLuaFile, {loadAPI = __lunaworld.loadAPI}))then
			doLunaworld = false
		end

		if(not loadCodeFile(__lunalocal, localLuaFile, {loadAPI = __lunalocal.loadAPI}))then
			doLunadll = false
		end
		
		return doLunaworld, doLunadll
	end)}
	
	if(not status[1])then
		windowDebug(status[2])
		error()
	end
	
	table.remove(status, 1)
	
	isLunaworld, isLunadll = unpack(status)
	
	
	if((not isLunaworld) and (not isLunadll))then
		error() --Shutdown Lua module as it is not used.
	end
	
end

-- Loads shared apis
function loadSharedAPI(api)
	if(__loadedAPIs[api])then
		return __loadedAPIs[api], false
	end
	
	local loadedAPI = doAPI(api)
	__loadedAPIs[api] = loadedAPI
	if(type(loadedAPI["onInitAPI"])=="function")then
		loadedAPI.onInitAPI()
	end
	return loadedAPI, true
end

function loadLocalAPI(apiHoster, api)
	local tHoster = nil
	if(apiHoster == "lunadll")then
		tHoster = __lunalocal
	elseif(apiHoster == "lunaworld")then
		tHoster = __lunaworld
	else
		return nil, false
	end
	
	if(tHoster.__loadedAPIs[api])then
		return tHoster.__loadedAPIs[api], false
	end
	
	local loadedAPI = doAPI(api)

	tHoster.__loadedAPIs[api] = loadedAPI
	if(type(loadedAPI["onInitAPI"])=="function")then
		loadedAPI.onInitAPI()
	end
	return loadedAPI, true
end

--[[
value = api
]]
function registerEvent(apiTable, event, eventHandler, beforeMainCall)
	if(type(apiTable)=="string")then
		error("\nOutdated version of API is trying to use registerEvent with string\nPlease contact the api developer to fix this issue!",2)
	end
	eventHandler = eventHandler or event
	beforeMainCall = beforeMainCall or true
	if(isAPILoaded(apiTable))then
		if(beforeMainCall)then
			if(not eventManager.eventHosterBefore[event])then
				eventManager.eventHosterBefore[event] = {}
			end
			local tHandler = {}
			tHandler.eventHandler = eventHandler
			tHandler.apiTable = apiTable
			table.insert(eventManager.eventHosterBefore[event], tHandler)
		else
			if(not eventManager.eventHosterAfter[event])then
				eventManager.eventHosterAfter[event] = {}
			end
			local tHandler = {}
			tHandler.eventHandler = eventHandler
			tHandler.apiTable = apiTable
			table.insert(eventManager.eventHosterAfter[event], tHandler)
		end
	end	
	
end

--Event Manager
eventManager = setmetatable({ --Normal table
	
	nextEvent = "",
	eventHosterBefore = {},
	eventHosterAfter = {},
	
	manageEvent = function(...)
		
		local eventReturn = nil
	
		--Event host before	
		if(eventManager.eventHosterBefore[eventManager.nextEvent])then
			for k,v in pairs(eventManager.eventHosterBefore[eventManager.nextEvent]) do
				if(type(v.apiTable[v.eventHandler])=="function")then
					local returns = {__xpcall(v.apiTable[v.eventHandler],...)} --Call event
					if(not returns[1])then
						windowDebug(returns[2])
						error()
					end
				end
			end
		end
		--Call global script event
		
		if(__lunaworld.__init)then
			if(type(__lunaworld[eventManager.nextEvent])=="function")then
				local returns = {__xpcall(__lunaworld[eventManager.nextEvent],...)}
				if(not returns[1])then
					windowDebug(returns[2])
					error()
				end
				table.remove(returns, 1)
				eventReturn = returns
			end
		end
		
		if(__lunalocal.__init)then
			if(type(__lunalocal[eventManager.nextEvent])=="function")then
				local returns = {__xpcall(__lunalocal[eventManager.nextEvent],...)}
				if(not returns[1])then
					windowDebug(returns[2])
					error()
				end
				table.remove(returns, 1)
				eventReturn = returns
			end
		end
		
		--Event host after
		if(eventManager.eventHosterAfter[eventManager.nextEvent])then
			for k,v in pairs(eventManager.eventHosterAfter[eventManager.nextEvent]) do
				if(type(v.apiTable[v.eventHandler])=="function")then
					local returns = {__xpcall(v.apiTable[v.eventHandler],...)} --Call event
					if(not returns[1])then
						windowDebug(returns[2])
						error()
					end
				end
			end
		end
		
		if(type(eventReturn) == "table")then
			return unpack(eventReturn)
		end
		return eventReturn
	end
},	
{ --Metatable
	__newindex = function (tarTable, key, value)
		--windowDebug("newindex: "..tostring(key))
	end,

	__index = function (tarTable, tarKey)
		if(type(rawget(eventManager,tarKey))=="nil")then --Handle a event
			rawset(eventManager, "nextEvent", tarKey)
			return rawget(eventManager, "manageEvent")
		else
			return rawget(eventManager, tarKey) --Get a already set field
		end
	end
});