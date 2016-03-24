#include <string>
#include "../../LuaProxy.h"
#include "../../../Misc/RuntimeHook.h"
#include "../../../GlobalFuncs.h"

std::string LuaProxy::Native::getSMBXPath()
{
    return WStr2Str(std::wstring(getModulePath()));
}

std::string LuaProxy::Native::getWorldPath()
{
	return (std::string)(VB6StrPtr)GM_FULLDIR;
}

void LuaProxy::Native::simulateError(short errcode)
{
    emulateVB6Error((int)errcode);
}