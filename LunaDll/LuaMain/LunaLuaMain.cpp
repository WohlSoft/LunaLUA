#include <string>
#include <memory>
#include <luabind/adopt_policy.hpp>
#include <luabind/out_value_policy.hpp>


#include "LunaLuaMain.h"
#include "../GlobalFuncs.h"
#include "../SMBXInternal/Level.h"
#include "../Misc/MiscFuncs.h"
#include "../SMBXInternal/PlayerMOB.h"
#include "../SMBXInternal/NPCs.h"

#include "../Rendering/FrameCapture.h"

#include "LuaHelper.h"
#include "LuaProxy.h"
#include "LuaProxyComponent/LuaProxyAudio.h"
#include "../libs/luasocket/luasocket.h"
#include "../libs/luasocket/mime.h"
#include "../SdlMusic/MusicManager.h"
#include "../Rendering/SMBXMaskedImage.h"


const std::wstring CLunaLua::LuaLibsPath = L"\\LuaScriptsLib\\mainV2.lua";
using namespace luabind;

std::wstring CLunaLua::getLuaLibsPath()
{
    std::wstring lapi = getModulePath();
    lapi = lapi.append(L"\\LuaScriptsLib\\mainV2.lua");
    return lapi;
}


CLunaLua::CLunaLua() :
        m_type(CLunaLua::LUNALUA_LEVEL),
        m_luaEventTableName(""),
        L(0),
        m_ready(false),
        m_eventLoopOnceExecuted(false),
        m_warningList()
{}

CLunaLua::~CLunaLua()
{
    //Just to be safe
    shutdown();
}

void CLunaLua::exitLevel()
{
    if (isValid())
    {
        std::shared_ptr<Event> shutdownEvent = std::make_shared<Event>("onExitLevel", false);
        shutdownEvent->setDirectEventName("onExitLevel");
        shutdownEvent->setLoopable(false);
        callEvent(shutdownEvent);
        shutdown();

        //Clean & stop all user started sounds and musics
        PGE_MusPlayer::MUS_stopMusic();
        PGE_Sounds::clearSoundBuffer();
    }
}

bool CLunaLua::shutdown()
{
    //Shutdown the lua module if possible
    if(!isValid())
        return false;
    
    checkWarnings();

    m_ready = false;
    m_eventLoopOnceExecuted = false;
    LuaProxy::Audio::resetMciSections();
	deletePassedLuaObj();
    lua_close(L);
    L = NULL;
    return true;
}

luabind::object* CLunaLua::registerPassedLuaObj(int index,lua_State* L,luabind::object& obj) {
	//test
	luabind::object* test = NULL;
#define PYPY luabind::object(L,obj,adopt(_2))
	switch (index+1) {
	case 1:
		obj1 = PYPY;
		test = &obj1;
		break;
	case 2:
		obj2 = PYPY;
		test = &obj2;
		break;
	case 3:
		obj3 = PYPY;
		test = &obj3;
		break;
	case 4:
		obj4 = PYPY;
		test = &obj4;
		break;
	case 5:
		obj5 = PYPY;
		test = &obj5;
		break;
	case 6:
		obj6 = PYPY;
		test = &obj6;
		break;
	case 7:
		obj7 = PYPY;
		test = &obj7;
		break;
	case 8:
		obj8 = PYPY;
		test = &obj8;
		break;
	default:
		break;
	}
#undef PYPY
	return test;

}
void CLunaLua::deletePassedLuaObj() {
	//testetstetstett
	//if (objList.empty())return;
	if (obj1.is_valid())obj1 = luabind::object();
	if (obj2.is_valid())obj2 = luabind::object();
	if (obj3.is_valid())obj3 = luabind::object();
	if (obj4.is_valid())obj4 = luabind::object();
	if (obj5.is_valid())obj5 = luabind::object();
	if (obj6.is_valid())obj6 = luabind::object();
	if (obj7.is_valid())obj7 = luabind::object();
	if (obj8.is_valid())obj8 = luabind::object();
	/*
	for (int i = 0; i < objList.size(); i++) {
		delete objList[i];
	}
	*/
	//objList.clear(); //DESTROYED
}

void CLunaLua::init(LuaLunaType type, std::wstring codePath, std::wstring levelPath /*= std::wstring()*/)
{
    SafeFPUControl noFPUExecptions;

    //Just to be safe
    shutdown();
    //Open up a new Lua State
    L = luaL_newstate();
    //Open all luabind functions
    luabind::open(L);
    //Set the new type
    m_type = type;

    //Open up "safe" standard lua libraries
    lua_pushcfunction(L, luaopen_base);
    lua_call(L,0,0);
    lua_pushcfunction(L, luaopen_math);
    lua_call(L,0,0);
    lua_pushcfunction(L, luaopen_string);
    lua_call(L,0,0);
    lua_pushcfunction(L, luaopen_table);
    lua_call(L,0,0);
    lua_pushcfunction(L, luaopen_debug);
    lua_call(L,0,0);
    lua_pushcfunction(L, luaopen_os);
    lua_call(L,0,0);
    lua_pushcfunction(L, luaopen_package);
    lua_call(L,0,0);
    lua_pushcfunction(L, luaopen_bit);
    lua_call(L, 0, 0);
    lua_pushcfunction(L, luaopen_ffi);
    lua_call(L, 0, 0);
    lua_pushcfunction(L, luaopen_jit);
    lua_call(L, 0, 0);

    //SOCKET TESTING STUFF
    lua_pushcfunction(L, luaopen_io);
    lua_call(L,0,0);

    lua_getfield(L, LUA_GLOBALSINDEX, "package");
    lua_getfield(L, -1, "preload");
    lua_pushcfunction(L, luaopen_socket_core);
    lua_setfield(L, -2, "socket.core");

    lua_pushcfunction(L, luaopen_mime_core);
    lua_setfield(L, -2, "mime.core");
    
    //SOCKET TESTING STUFF


    //Remove unsafe apis
    {
        object _G = globals(L);
        object osTable = _G["os"];
        osTable["execute"] = object();
        osTable["exit"] = object();
        //osTable["getenv"] = object();
        osTable["remove"] = object();
        osTable["rename"] = object();
        osTable["setlocal"] = object();
        osTable["tmpname"] = object();
    }

    //Read the API-File
    std::wstring wLuaCode;
    if(!readFile(wLuaCode, getLuaLibsPath(), L"Since v0.3 the LuaScriptsLib-Folder with\nall its content is required.\nBe sure you installed everything correctly!")){
        shutdown();
        return;
    }
    //Convert to ASCII, as lua doesn't support unicode
    std::string LuaCode = WStr2Str(wLuaCode);

    //Bind all functions, propeties ect...
    bindAll();
    bindAllDeprecated();

    //Setup default contants
    setupDefaults();

    //Load the Lua API
    bool errLapi = false;
    int lapierrcode = luaL_loadbuffer(L, LuaCode.c_str(), LuaCode.length(), "=mainV2.lua") || lua_pcall(L, 0, LUA_MULTRET, 0);
    if(!(lapierrcode == 0)){
        object error_msg(from_stack(L, -1));
        MessageBoxA(0, object_cast<const char*>(error_msg), "Error", MB_ICONWARNING);
        errLapi = true;
    }
    {
        errLapi = errLapi || luabind::object_cast<bool>(luabind::globals(L)["__isLuaError"]);
    }
    
    
    //Shutdown if an error happend.
    if(errLapi){
        shutdown();
        return;
    }

    //Call the lua api init funtion.
    const char* initName = "__onInit";
    if (LuaHelper::is_function(L, initName)) {
        callLuaFunction(L, initName, WStr2Str(codePath), WStr2Str(levelPath));
    }
}

//Setup default constants
void CLunaLua::setupDefaults()
{
    object _G = globals(L);
    LUAHELPER_DEF_CONST(_G, GAME_ENGINE);
    LUAHELPER_DEF_CONST(_G, LUNALUA_VERSION);
    _G["LUNALUA_VER"] = LUNALUA_VERSION; // ALIAS
    
    object verTable = newtable(L);
    verTable[1] = LUNA_VERNUM1;
    verTable[2] = LUNA_VERNUM2;
    verTable[3] = LUNA_VERNUM3;
    verTable[4] = LUNA_VERNUM4;
    _G["__LUNA_VERSION_TABLE"] = verTable;

    LUAHELPER_DEF_CONST(_G, PLAYER_SMALL);
    LUAHELPER_DEF_CONST(_G, PLAYER_BIG);
    LUAHELPER_DEF_CONST(_G, PLAYER_FIREFLOWER);
    LUAHELPER_DEF_CONST(_G, PLAYER_LEAF);
    LUAHELPER_DEF_CONST(_G, PLAYER_TANOOKIE);
    LUAHELPER_DEF_CONST(_G, PLAYER_HAMMER);
    LUAHELPER_DEF_CONST(_G, PLAYER_ICE);

    LUAHELPER_DEF_CONST(_G, CHARACTER_MARIO);
    LUAHELPER_DEF_CONST(_G, CHARACTER_LUIGI);
    LUAHELPER_DEF_CONST(_G, CHARACTER_PEACH);
    LUAHELPER_DEF_CONST(_G, CHARACTER_TOAD);
    LUAHELPER_DEF_CONST(_G, CHARACTER_LINK);

    _G["FIND_ANY"] = -1;

    _G["DIR_RIGHT"] = 1;
    _G["DIR_RANDOM"] = 0;
    _G["DIR_LEFT"] = -1;

    _G["FIELD_BYTE"] = LuaProxy::LFT_BYTE;
    _G["FIELD_WORD"] = LuaProxy::LFT_WORD;
    _G["FIELD_DWORD"] = LuaProxy::LFT_DWORD;
    _G["FIELD_FLOAT"] = LuaProxy::LFT_FLOAT;
    _G["FIELD_DFLOAT"] = LuaProxy::LFT_DFLOAT;
    _G["FIELD_STRING"] = LuaProxy::LFT_STRING;
    _G["FIELD_BOOL"] = LuaProxy::LFT_BOOL;

    _G["KEY_UP"] = GM_PLAYER_KEY_UP;
    _G["KEY_DOWN"] = GM_PLAYER_KEY_DOWN;
    _G["KEY_LEFT"] = GM_PLAYER_KEY_LEFT;
    _G["KEY_RIGHT"] = GM_PLAYER_KEY_RIGHT;
    _G["KEY_JUMP"] = GM_PLAYER_KEY_JUMP;
    _G["KEY_SPINJUMP"] = GM_PLAYER_KEY_SJUMP;
    _G["KEY_X"] = GM_PLAYER_KEY_X;
    _G["KEY_RUN"] = GM_PLAYER_KEY_RUN;
    _G["KEY_SEL"] = GM_PLAYER_KEY_SEL;
    _G["KEY_STR"] = GM_PLAYER_KEY_STR;

    LUAHELPER_DEF_CONST(_G, VK_LBUTTON);
    LUAHELPER_DEF_CONST(_G, VK_RBUTTON);
    LUAHELPER_DEF_CONST(_G, VK_CANCEL);
    LUAHELPER_DEF_CONST(_G, VK_MBUTTON);
    LUAHELPER_DEF_CONST(_G, VK_XBUTTON1);
    LUAHELPER_DEF_CONST(_G, VK_XBUTTON2);
    LUAHELPER_DEF_CONST(_G, VK_BACK);
    LUAHELPER_DEF_CONST(_G, VK_TAB);
    LUAHELPER_DEF_CONST(_G, VK_CLEAR);
    LUAHELPER_DEF_CONST(_G, VK_RETURN);
    LUAHELPER_DEF_CONST(_G, VK_SHIFT);
    LUAHELPER_DEF_CONST(_G, VK_CONTROL);
    LUAHELPER_DEF_CONST(_G, VK_MENU);
    LUAHELPER_DEF_CONST(_G, VK_PAUSE);
    LUAHELPER_DEF_CONST(_G, VK_CAPITAL);
    LUAHELPER_DEF_CONST(_G, VK_KANA);
    LUAHELPER_DEF_CONST(_G, VK_HANGEUL);
    LUAHELPER_DEF_CONST(_G, VK_HANGUL);
    LUAHELPER_DEF_CONST(_G, VK_JUNJA);
    LUAHELPER_DEF_CONST(_G, VK_FINAL);
    LUAHELPER_DEF_CONST(_G, VK_HANJA);
    LUAHELPER_DEF_CONST(_G, VK_KANJI);
    LUAHELPER_DEF_CONST(_G, VK_ESCAPE);
    LUAHELPER_DEF_CONST(_G, VK_CONVERT);
    LUAHELPER_DEF_CONST(_G, VK_NONCONVERT);
    LUAHELPER_DEF_CONST(_G, VK_ACCEPT);
    LUAHELPER_DEF_CONST(_G, VK_MODECHANGE);
    LUAHELPER_DEF_CONST(_G, VK_SPACE);
    LUAHELPER_DEF_CONST(_G, VK_PRIOR);
    LUAHELPER_DEF_CONST(_G, VK_NEXT);
    LUAHELPER_DEF_CONST(_G, VK_END);
    LUAHELPER_DEF_CONST(_G, VK_HOME);
    LUAHELPER_DEF_CONST(_G, VK_LEFT);
    LUAHELPER_DEF_CONST(_G, VK_UP);
    LUAHELPER_DEF_CONST(_G, VK_RIGHT);
    LUAHELPER_DEF_CONST(_G, VK_DOWN);
    LUAHELPER_DEF_CONST(_G, VK_SELECT);
    LUAHELPER_DEF_CONST(_G, VK_PRINT);
    LUAHELPER_DEF_CONST(_G, VK_EXECUTE);
    LUAHELPER_DEF_CONST(_G, VK_SNAPSHOT);
    LUAHELPER_DEF_CONST(_G, VK_INSERT);
    LUAHELPER_DEF_CONST(_G, VK_DELETE);
    LUAHELPER_DEF_CONST(_G, VK_HELP);

    // VK_# (# from 0 to 9)
    std::string vkConstantName = "VK_";
    for (char i = '0'; i <= '9'; i++) {
        std::string nextName = vkConstantName + i;
        _G[nextName] = i;
    }

    // VK_# (# from A to Z)
    for (char i = 'A'; i <= 'Z'; i++) {
        std::string nextName = vkConstantName + i;
        _G[nextName] = i;
    }

    LUAHELPER_DEF_CONST(_G, VK_LWIN);
    LUAHELPER_DEF_CONST(_G, VK_RWIN);
    LUAHELPER_DEF_CONST(_G, VK_APPS);
    LUAHELPER_DEF_CONST(_G, VK_SLEEP);
    LUAHELPER_DEF_CONST(_G, VK_NUMPAD0);
    LUAHELPER_DEF_CONST(_G, VK_NUMPAD1);
    LUAHELPER_DEF_CONST(_G, VK_NUMPAD2);
    LUAHELPER_DEF_CONST(_G, VK_NUMPAD3);
    LUAHELPER_DEF_CONST(_G, VK_NUMPAD4);
    LUAHELPER_DEF_CONST(_G, VK_NUMPAD5);
    LUAHELPER_DEF_CONST(_G, VK_NUMPAD6);
    LUAHELPER_DEF_CONST(_G, VK_NUMPAD7);
    LUAHELPER_DEF_CONST(_G, VK_NUMPAD8);
    LUAHELPER_DEF_CONST(_G, VK_NUMPAD9);
    LUAHELPER_DEF_CONST(_G, VK_MULTIPLY);
    LUAHELPER_DEF_CONST(_G, VK_ADD);
    LUAHELPER_DEF_CONST(_G, VK_SEPARATOR);
    LUAHELPER_DEF_CONST(_G, VK_SUBTRACT);
    LUAHELPER_DEF_CONST(_G, VK_DECIMAL);
    LUAHELPER_DEF_CONST(_G, VK_DIVIDE);
    LUAHELPER_DEF_CONST(_G, VK_F1);
    LUAHELPER_DEF_CONST(_G, VK_F2);
    LUAHELPER_DEF_CONST(_G, VK_F3);
    LUAHELPER_DEF_CONST(_G, VK_F4);
    LUAHELPER_DEF_CONST(_G, VK_F5);
    LUAHELPER_DEF_CONST(_G, VK_F6);
    LUAHELPER_DEF_CONST(_G, VK_F7);
    LUAHELPER_DEF_CONST(_G, VK_F8);
    LUAHELPER_DEF_CONST(_G, VK_F9);
    LUAHELPER_DEF_CONST(_G, VK_F10);
    LUAHELPER_DEF_CONST(_G, VK_F11);
    LUAHELPER_DEF_CONST(_G, VK_F12);
    LUAHELPER_DEF_CONST(_G, VK_F13);
    LUAHELPER_DEF_CONST(_G, VK_F14);
    LUAHELPER_DEF_CONST(_G, VK_F15);
    LUAHELPER_DEF_CONST(_G, VK_F16);
    LUAHELPER_DEF_CONST(_G, VK_F17);
    LUAHELPER_DEF_CONST(_G, VK_F18);
    LUAHELPER_DEF_CONST(_G, VK_F19);
    LUAHELPER_DEF_CONST(_G, VK_F20);
    LUAHELPER_DEF_CONST(_G, VK_F21);
    LUAHELPER_DEF_CONST(_G, VK_F22);
    LUAHELPER_DEF_CONST(_G, VK_F23);
    LUAHELPER_DEF_CONST(_G, VK_F24);
    LUAHELPER_DEF_CONST(_G, VK_NUMLOCK);
    LUAHELPER_DEF_CONST(_G, VK_SCROLL);
    LUAHELPER_DEF_CONST(_G, VK_OEM_NEC_EQUAL);
    LUAHELPER_DEF_CONST(_G, VK_OEM_FJ_JISHO);
    LUAHELPER_DEF_CONST(_G, VK_OEM_FJ_MASSHOU);
    LUAHELPER_DEF_CONST(_G, VK_OEM_FJ_TOUROKU);
    LUAHELPER_DEF_CONST(_G, VK_OEM_FJ_LOYA);
    LUAHELPER_DEF_CONST(_G, VK_OEM_FJ_ROYA);
    LUAHELPER_DEF_CONST(_G, VK_LSHIFT);
    LUAHELPER_DEF_CONST(_G, VK_RSHIFT);
    LUAHELPER_DEF_CONST(_G, VK_LCONTROL);
    LUAHELPER_DEF_CONST(_G, VK_RCONTROL);
    LUAHELPER_DEF_CONST(_G, VK_LMENU);
    LUAHELPER_DEF_CONST(_G, VK_RMENU);
    LUAHELPER_DEF_CONST(_G, VK_BROWSER_BACK);
    LUAHELPER_DEF_CONST(_G, VK_BROWSER_FORWARD);
    LUAHELPER_DEF_CONST(_G, VK_BROWSER_REFRESH);
    LUAHELPER_DEF_CONST(_G, VK_BROWSER_STOP);
    LUAHELPER_DEF_CONST(_G, VK_BROWSER_SEARCH);
    LUAHELPER_DEF_CONST(_G, VK_BROWSER_FAVORITES);
    LUAHELPER_DEF_CONST(_G, VK_BROWSER_HOME);
    LUAHELPER_DEF_CONST(_G, VK_VOLUME_MUTE);
    LUAHELPER_DEF_CONST(_G, VK_VOLUME_DOWN);
    LUAHELPER_DEF_CONST(_G, VK_VOLUME_UP);
    LUAHELPER_DEF_CONST(_G, VK_MEDIA_NEXT_TRACK);
    LUAHELPER_DEF_CONST(_G, VK_MEDIA_PREV_TRACK);
    LUAHELPER_DEF_CONST(_G, VK_MEDIA_STOP);
    LUAHELPER_DEF_CONST(_G, VK_MEDIA_PLAY_PAUSE);
    LUAHELPER_DEF_CONST(_G, VK_LAUNCH_MAIL);
    LUAHELPER_DEF_CONST(_G, VK_LAUNCH_MEDIA_SELECT);
    LUAHELPER_DEF_CONST(_G, VK_LAUNCH_APP1);
    LUAHELPER_DEF_CONST(_G, VK_LAUNCH_APP2);

    LUAHELPER_DEF_CONST(_G, WHUD_ALL);
    LUAHELPER_DEF_CONST(_G, WHUD_ONLY_OVERLAY);
    LUAHELPER_DEF_CONST(_G, WHUD_NONE);

    LUAHELPER_DEF_CONST(_G, EXITTYPE_ANY);
    LUAHELPER_DEF_CONST(_G, EXITTYPE_BOSS);
    LUAHELPER_DEF_CONST(_G, EXITTYPE_CARD_ROULETTE);
    LUAHELPER_DEF_CONST(_G, EXITTYPE_CRYSTAL);
    LUAHELPER_DEF_CONST(_G, EXITTYPE_NONE);
    LUAHELPER_DEF_CONST(_G, EXITTYPE_OFFSCREEN);
    LUAHELPER_DEF_CONST(_G, EXITTYPE_SECRET);
    LUAHELPER_DEF_CONST(_G, EXITTYPE_STAR);
    LUAHELPER_DEF_CONST(_G, EXITTYPE_TAPE);
    LUAHELPER_DEF_CONST(_G, EXITTYPE_WARP);

    LUAHELPER_DEF_CONST(_G, HARM_TYPE_JUMP);
    LUAHELPER_DEF_CONST(_G, HARM_TYPE_FROMBELOW);
    LUAHELPER_DEF_CONST(_G, HARM_TYPE_NPC);
    LUAHELPER_DEF_CONST(_G, HARM_TYPE_PROJECTILE_USED);
    LUAHELPER_DEF_CONST(_G, HARM_TYPE_LAVA);
    LUAHELPER_DEF_CONST(_G, HARM_TYPE_HELD);
    LUAHELPER_DEF_CONST(_G, HARM_TYPE_TAIL);
    LUAHELPER_DEF_CONST(_G, HARM_TYPE_SPINJUMP);
    LUAHELPER_DEF_CONST(_G, HARM_TYPE_OFFSCREEN);
    LUAHELPER_DEF_CONST(_G, HARM_TYPE_SWORD);
    LUAHELPER_DEF_CONST(_G, HARM_TYPE_EXT_FIRE);
    LUAHELPER_DEF_CONST(_G, HARM_TYPE_EXT_ICE);
    LUAHELPER_DEF_CONST(_G, HARM_TYPE_EXT_HAMMER);

    {
        using namespace LuaProxy::Graphics;
        LUAHELPER_DEF_CONST(_G, RTYPE_IMAGE);
        LUAHELPER_DEF_CONST(_G, RTYPE_TEXT);
    }

    _G["ODIR_UP"] = 1;
    _G["ODIR_LEFT"] = 2;
    _G["ODIR_DOWN"] = 3;
    _G["ODIR_RIGHT"] = 4;
    if(m_type == LUNALUA_WORLD){
        _G["isOverworld"] = true;
        _G["world"] = LuaProxy::World();
    }
    
    _G["player"] = LuaProxy::Player();
    LuaProxy::Player pl(2);
    if(pl.isValid())
        _G["player2"] = pl;

    _G["inputConfig1"] = LuaProxy::InputConfig(1);
    _G["inputConfig2"] = LuaProxy::InputConfig(2);
    
    _G["console"] = LuaProxy::Console();
}

LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Graphics::LuaImageResource, LuaImageResource);
LUAHELPER_DEF_CLASS_HELPER(CaptureBuffer, CaptureBuffer);
LUAHELPER_DEF_CLASS_HELPER(SMBXMaskedImage, SMBXMaskedImage);
LUAHELPER_DEF_CLASS_HELPER(Mix_Chunk, Mix_Chunk);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::InputConfig, NativeInputConfig);
LUAHELPER_DEF_CLASS_HELPER(RECT, RECT);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::RECTd, RECTd);
LUAHELPER_DEF_CLASS_HELPER(Event, Event);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Logger, Logger);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Data, Data);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::AsyncHTTPRequest, AsyncHTTPRequest);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::PlayerSettings, PlayerSettings);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Player, Player);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Camera, Camera);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::VBStr, VBStr);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::World, World);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Tile, Tile);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Scenery, Scenery);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Path, Path);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Musicbox, Musicbox);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::LevelObject, Level);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Warp, Warp);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Animation, Animation);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Layer, Layer);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Section, Section);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::NPC, NPC);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::Block, Block);
LUAHELPER_DEF_CLASS_HELPER(LuaProxy::BGO, BGO);


// LUAHELPER_DEF_CLASS(LuaImageResource)


void CLunaLua::bindAll()
{
    //Bind stuff for world and level
    module(L)
        [
            def("mem", (void(*)(int, LuaProxy::L_FIELDTYPE, const luabind::object &, lua_State*)) &LuaProxy::mem),
            def("mem", (luabind::object(*)(int, LuaProxy::L_FIELDTYPE, lua_State*)) &LuaProxy::mem),

            namespace_("Native")[
                def("getSMBXPath", &LuaProxy::Native::getSMBXPath),
				def("getWorldPath", &LuaProxy::Native::getWorldPath),
                def("simulateError", &LuaProxy::Native::simulateError)
            ],

            namespace_("Text")[
				def("windowDebug", &LuaProxy::Text::windowDebug),
					def("windowDebugSimple", &LuaProxy::Text::windowDebugSimple),
					def("print", (void(*)(const luabind::object&, int, int)) &LuaProxy::Text::print),
					def("print", (void(*)(const luabind::object&, int, int, int)) &LuaProxy::Text::print),
					def("printWP", (void(*)(const luabind::object&, int, int, double)) &LuaProxy::Text::printWP),
					def("printWP", (void(*)(const luabind::object&, int, int, int, double)) &LuaProxy::Text::printWP)
			],
					
            namespace_("Graphics")[
				LUAHELPER_DEF_CLASS(LuaImageResource)
					.def("__eq", &LuaProxy::luaUserdataCompare<LuaProxy::Graphics::LuaImageResource>)
					.def("setScaleUpMode", &LuaProxy::Graphics::LuaImageResource::setScaleUpMode)
					.def("setScaleDownMode", &LuaProxy::Graphics::LuaImageResource::setScaleDownMode)
					.property("width", &LuaProxy::Graphics::LuaImageResource::GetWidth)
					.property("height", &LuaProxy::Graphics::LuaImageResource::GetHeight)
					.property("__BMPBoxPtr", &LuaProxy::Graphics::LuaImageResource::__BMPBoxPtr),
					class_<LuaProxy::Graphics::LuaMovieResource, LuaProxy::Graphics::LuaImageResource>("LuaMovieResource")
					.def("setMaskThreshold", &LuaProxy::Graphics::LuaMovieResource::setMaskThreshold)
					.def("getMaskThreshold", &LuaProxy::Graphics::LuaMovieResource::getMaskThreshold)
					.def("play", &LuaProxy::Graphics::LuaMovieResource::play)
					.def("stop", &LuaProxy::Graphics::LuaMovieResource::stop)
					.def("pause", &LuaProxy::Graphics::LuaMovieResource::pause)
					.def("seek", &LuaProxy::Graphics::LuaMovieResource::seek)
					.property("videoDelay", &LuaProxy::Graphics::LuaMovieResource::getVideoDelay, &LuaProxy::Graphics::LuaMovieResource::setVideoDelay)
					.property("maskDelay", &LuaProxy::Graphics::LuaMovieResource::getMaskDelay, &LuaProxy::Graphics::LuaMovieResource::setMaskDelay)
					.property("scaleUpMode", &LuaProxy::Graphics::LuaMovieResource::getScaleUpMode, &LuaProxy::Graphics::LuaMovieResource::setScaleUpMode)
					.property("scaleDownMode", &LuaProxy::Graphics::LuaMovieResource::getScaleDownMode, &LuaProxy::Graphics::LuaMovieResource::setScaleDownMode)
					.property("offScreenMode", &LuaProxy::Graphics::LuaMovieResource::getOffScreenMode, &LuaProxy::Graphics::LuaMovieResource::setOffScreenMode)
					.property("onScreenMode", &LuaProxy::Graphics::LuaMovieResource::getOnScreenMode, &LuaProxy::Graphics::LuaMovieResource::setOnScreenMode)
					.property("loop", &LuaProxy::Graphics::LuaMovieResource::getLoop, &LuaProxy::Graphics::LuaMovieResource::setLoop)
					.property("altAlpha", &LuaProxy::Graphics::LuaMovieResource::getAltAlpha, &LuaProxy::Graphics::LuaMovieResource::setAltAlpha)
					.property("alphaType", &LuaProxy::Graphics::LuaMovieResource::getAlphaType, &LuaProxy::Graphics::LuaMovieResource::setAlphaType)
					.property("volume", &LuaProxy::Graphics::LuaMovieResource::getVolume, &LuaProxy::Graphics::LuaMovieResource::setVolume)
					,
                LUAHELPER_DEF_CLASS(SMBXMaskedImage)
                    .def("__eq", &LuaProxy::luaUserdataCompare<SMBXMaskedImage>),
                LUAHELPER_DEF_CLASS_SMART_PTR_SHARED(CaptureBuffer, std::shared_ptr)
                    .def(constructor<int, int>())
                    .def("__eq", &LuaProxy::luaUserdataCompare<LuaProxy::Graphics::LuaImageResource>)
                    .def("captureAt", &CaptureBuffer::captureAt),
                def("loadImage", (bool(*)(const std::string&, int, int))&LuaProxy::Graphics::loadImage),
                def("loadImage", (LuaProxy::Graphics::LuaImageResource*(*)(const std::string&, lua_State*))&LuaProxy::Graphics::loadImage, adopt(result)),
				def("loadMovie", (LuaProxy::Graphics::LuaMovieResource*(*)(const std::string&, lua_State*))&LuaProxy::Graphics::loadMovie, adopt(result)),
				def("loadAnimatedImage", &LuaProxy::Graphics::loadAnimatedImage, pure_out_value(_2)),
                def("placeSprite", (void(*)(int, int, int, int, const std::string&, int))&LuaProxy::Graphics::placeSprite),
                def("placeSprite", (void(*)(int, int, int, int, const std::string&))&LuaProxy::Graphics::placeSprite),
                def("placeSprite", (void(*)(int, int, int, int))&LuaProxy::Graphics::placeSprite),
                def("placeSprite", (void(*)(int, const LuaProxy::Graphics::LuaImageResource& img, int, int, const std::string&, int))&LuaProxy::Graphics::placeSprite),
                def("placeSprite", (void(*)(int, const LuaProxy::Graphics::LuaImageResource& img, int, int, const std::string&))&LuaProxy::Graphics::placeSprite),
                def("placeSprite", (void(*)(int, const LuaProxy::Graphics::LuaImageResource& img, int, int))&LuaProxy::Graphics::placeSprite),
                def("unplaceSprites", (void(*)(const LuaProxy::Graphics::LuaImageResource& img))&LuaProxy::Graphics::unplaceSprites),
                def("unplaceSprites", (void(*)(const LuaProxy::Graphics::LuaImageResource& img, int, int))&LuaProxy::Graphics::unplaceSprites),
                def("getPixelData", &LuaProxy::Graphics::getPixelData, pure_out_value(_2) + pure_out_value(_3)),
                def("drawImage", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, lua_State*))&LuaProxy::Graphics::drawImage),
                def("drawImage", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, float, lua_State*))&LuaProxy::Graphics::drawImage),
                def("drawImage", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, double, double, double, lua_State*))&LuaProxy::Graphics::drawImage),
                def("drawImage", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, double, double, double, float, lua_State*))&LuaProxy::Graphics::drawImage),
                def("drawImageWP", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, lua_State*))&LuaProxy::Graphics::drawImageWP),
                def("drawImageWP", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, float, double, lua_State*))&LuaProxy::Graphics::drawImageWP),
                def("drawImageWP", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, double, double, double, double, lua_State*))&LuaProxy::Graphics::drawImageWP),
                def("drawImageWP", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, double, double, double, float, double, lua_State*))&LuaProxy::Graphics::drawImageWP),
                def("drawImageToScene", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, lua_State*))&LuaProxy::Graphics::drawImageToScene),
                def("drawImageToScene", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, float, lua_State*))&LuaProxy::Graphics::drawImageToScene),
                def("drawImageToScene", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, double, double, double, lua_State*))&LuaProxy::Graphics::drawImageToScene),
                def("drawImageToScene", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, double, double, double, float, lua_State*))&LuaProxy::Graphics::drawImageToScene),
                def("drawImageToSceneWP", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, lua_State*))&LuaProxy::Graphics::drawImageToSceneWP),
                def("drawImageToSceneWP", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, float, double, lua_State*))&LuaProxy::Graphics::drawImageToSceneWP),
                def("drawImageToSceneWP", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, double, double, double, double, lua_State*))&LuaProxy::Graphics::drawImageToSceneWP),
                def("drawImageToSceneWP", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, double, double, double, float, double, lua_State*))&LuaProxy::Graphics::drawImageToSceneWP),

				//add
				
				def("drawImageScaleWP", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, double, double, lua_State*))&LuaProxy::Graphics::drawImageScaleWP),
				def("drawImageScaleWP", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, double, float,double, lua_State*))&LuaProxy::Graphics::drawImageScaleWP),
				//too many args
				//def("drawImageScaleWP", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, double, double,double, double,double,double, lua_State*))&LuaProxy::Graphics::drawImageScaleWP),
				
				//def("drawImageScaleWP", (void(*)(const LuaProxy::Graphics::LuaImageResource&, double, double, double, double, double, double,double,double, float,double, lua_State*))&LuaProxy::Graphics::drawImageScaleWP),
				
                def("draw", &LuaProxy::Graphics::draw),
                def("isOpenGLEnabled", &LuaProxy::Graphics::isOpenGLEnabled),
                def("glSetTexture", &LuaProxy::Graphics::glSetTexture),
                def("glSetTextureRGBA", &LuaProxy::Graphics::glSetTextureRGBA),
                // glDrawTriangles will be defined at runtime using FFI
                def("__glInternalDraw", &LuaProxy::Graphics::__glInternalDraw),
                def("__setSpriteOverride", &LuaProxy::Graphics::__setSpriteOverride),
                def("__setSimpleSpriteOverride", &LuaProxy::Graphics::__setSimpleSpriteOverride),
                def("__getSpriteOverride", &LuaProxy::Graphics::__getSpriteOverride)
            ],

            namespace_("__Effects_EXPERIMENTAL")[
                def("screenGlow", &LuaProxy::Effects::screenGlow),
                def("screenGlowNegative", &LuaProxy::Effects::screenGlowNegative),
                def("flipX", &LuaProxy::Effects::flipX),
                def("flipY", &LuaProxy::Effects::flipY),
                def("flipXY", &LuaProxy::Effects::flipXY)
            ],

            namespace_("Misc")[
                def("cheatBuffer", (std::string(*)())&LuaProxy::Misc::cheatBuffer),
                def("cheatBuffer", (void(*)(const luabind::object&, lua_State*))&LuaProxy::Misc::cheatBuffer),
                def("listFiles", &LuaProxy::Misc::listFiles),
                def("listLocalFiles", &LuaProxy::Misc::listLocalFiles),
                def("resolveFile", &LuaProxy::Misc::resolveFile),
                def("resolveDirectory", &LuaProxy::Misc::resolveDirectory),
                def("isSamePath", &LuaProxy::Misc::isSamePath),
                def("openPauseMenu", &LuaProxy::Misc::openPauseMenu),
                def("saveGame", &LuaProxy::Misc::saveGame),
                def("exitGame", &LuaProxy::Misc::exitGame),
                def("loadEpisode", &LuaProxy::Misc::loadEpisode),
                def("pause", &LuaProxy::Misc::pause),
                def("unpause", &LuaProxy::Misc::unpause),
                def("isPausedByLua", &LuaProxy::Misc::isPausedByLua),
                def("warning", &LuaProxy::Misc::warning),
                def("registerCharacterId", &LuaProxy::Misc::registerCharacterId),
                def("warning", &LuaProxy::Misc::warning),
				def("updateWarp", &LuaProxy::Misc::updateWarp)
            ],

            namespace_("Audio")[
                //SDL_Mixer's Mix_Chunk structure
                LUAHELPER_DEF_CLASS(Mix_Chunk)
                    .property("allocated", &Mix_Chunk::allocated)
                    .property("abuf", &Mix_Chunk::abuf)
                    .def_readwrite("alen", &Mix_Chunk::alen)
                    .def_readwrite("volume", &Mix_Chunk::volume),

                //Music
                def("MusicOpen", (void(*)(const std::string&))&LuaProxy::Audio::MusicOpen),
                def("MusicPlay", (void(*)())&LuaProxy::Audio::MusicPlay),
                def("MusicPlayFadeIn", (void(*)(int))&LuaProxy::Audio::MusicPlayFadeIn),
                def("MusicStop", (void(*)())&LuaProxy::Audio::MusicStop),
                def("MusicStopFadeOut", (void(*)(int))&LuaProxy::Audio::MusicStopFadeOut),
                def("MusicPause", (void(*)())&LuaProxy::Audio::MusicPause),
                def("MusicResume", (void(*)())&LuaProxy::Audio::MusicResume),
                def("MusicIsPlaying", (bool(*)())&LuaProxy::Audio::MusicIsPlaying),
                def("MusicIsPaused", (bool(*)())&LuaProxy::Audio::MusicIsPaused),
                def("MusicIsFading", (bool(*)())&LuaProxy::Audio::MusicIsFading),
                def("MusicVolume", (void(*)(int))&LuaProxy::Audio::MusicVolume),
                def("MusicTitle", (std::string(*)())&LuaProxy::Audio::MusicTitle),
                def("MusicTitleTag", (std::string(*)())&LuaProxy::Audio::MusicTitleTag),
                def("MusicArtistTag", (std::string(*)())&LuaProxy::Audio::MusicArtistTag),
                def("MusicAlbumTag", (std::string(*)())&LuaProxy::Audio::MusicAlbumTag),
                def("MusicCopyrightTag", (std::string(*)())&LuaProxy::Audio::MusicCopyrightTag),
                //Seize music stream for LUA usage for section 0..20
                def("SeizeStream", (void(*)(int))&LuaProxy::Audio::seizeStream),
                //Return music stream access to SMBX engine back for section 0..20
                def("ReleaseStream", (void(*)(int))&LuaProxy::Audio::releaseStream),
                //Release music stream for ALL sections
                def("resetMciSections", (void(*)())&LuaProxy::Audio::resetMciSections),

                //SFX
                def("newMix_Chunk", (Mix_Chunk*(*)())&LuaProxy::Audio::newMix_Chunk),
                def("clearSFXBuffer", (void(*)())&LuaProxy::Audio::clearSFXBuffer),
                def("playSFX", (void(*)(const std::string&))&LuaProxy::Audio::playSFX),
                def("SfxOpen", (Mix_Chunk*(*)(const std::string&))&LuaProxy::Audio::SfxOpen),
                def("SfxPlayCh", (int(*)(int, Mix_Chunk*,int))&LuaProxy::Audio::SfxPlayCh),
                def("SfxPlayChTimed", (int(*)(int, Mix_Chunk*, int, int))&LuaProxy::Audio::SfxPlayChTimed),
                def("SfxFadeInCh", (int(*)(int, Mix_Chunk*, int, int))&LuaProxy::Audio::SfxFadeInCh),
                def("SfxFadeInChTimed", (int(*)(int, Mix_Chunk*, int, int, int))&LuaProxy::Audio::SfxFadeInChTimed),
                def("SfxPause", (void(*)(int))&LuaProxy::Audio::SfxPause),
                def("SfxResume", (void(*)(int))&LuaProxy::Audio::SfxResume),
                def("SfxStop", (int(*)(int))&LuaProxy::Audio::SfxStop),
                def("SfxExpire", (int(*)(int, int))&LuaProxy::Audio::SfxExpire),
                def("SfxFadeOut", (int(*)(int, int))&LuaProxy::Audio::SfxFadeOut),
                def("SfxIsPlaying", (int(*)(int))&LuaProxy::Audio::SfxIsPlaying),
                def("SfxIsPaused", (int(*)(int))&LuaProxy::Audio::SfxIsPaused),
                def("SfxIsFading", (int(*)(int))&LuaProxy::Audio::SfxIsFading),
                def("SfxVolume", (int(*)(int, int))&LuaProxy::Audio::SfxVolume),

                def("SfxSetPanning", (int(*)(int, int, int))&LuaProxy::Audio::SfxSetPanning),
                def("SfxSetDistance", (int(*)(int, int))&LuaProxy::Audio::SfxSetDistance),
                def("SfxSet3DPosition", (int(*)(int, int, int))&LuaProxy::Audio::SfxSet3DPosition),
                def("SfxReverseStereo", (int(*)(int, int))&LuaProxy::Audio::SfxReverseStereo),

                //Time
                def("AudioClock", (double(*)())&LuaProxy::Audio::AudioClock),
                def("MusicClock", (double(*)())&LuaProxy::Audio::MusicClock),

                def("__setOverrideForAlias", LuaProxy::Audio::__setOverrideForAlias),
                def("__getChunkForAlias", LuaProxy::Audio::__getChunkForAlias),
                def("__setMuteForAlias", LuaProxy::Audio::__setMuteForAlias),
                def("__getMuteForAlias", LuaProxy::Audio::__getMuteForAlias)
            ],
            /*************************Audio*end*************************/

            LUAHELPER_DEF_CLASS(NativeInputConfig)
            .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::InputConfig, m_index))
            .property("inputType", &LuaProxy::InputConfig::inputType, &LuaProxy::InputConfig::setInputType)
            .property("down", &LuaProxy::InputConfig::down, &LuaProxy::InputConfig::setDown)
            .property("left", &LuaProxy::InputConfig::left, &LuaProxy::InputConfig::setLeft)
            .property("right", &LuaProxy::InputConfig::right, &LuaProxy::InputConfig::setRight)
            .property("run", &LuaProxy::InputConfig::run, &LuaProxy::InputConfig::setRun)
            .property("altrun", &LuaProxy::InputConfig::altrun, &LuaProxy::InputConfig::setAltRun)
            .property("jump", &LuaProxy::InputConfig::jump, &LuaProxy::InputConfig::setJump)
            .property("altjump", &LuaProxy::InputConfig::altjump, &LuaProxy::InputConfig::setAltJump)
            .property("dropitem", &LuaProxy::InputConfig::dropitem, &LuaProxy::InputConfig::setDropItem)
            .property("pause", &LuaProxy::InputConfig::pause, &LuaProxy::InputConfig::setPause),

            LUAHELPER_DEF_CLASS(RECT)
            .def_readwrite("left", &RECT::left)
            .def_readwrite("top", &RECT::top)
            .def_readwrite("right", &RECT::right)
            .def_readwrite("bottom", &RECT::bottom),

            LUAHELPER_DEF_CLASS(RECTd)
            .def_readwrite("left", &LuaProxy::RECTd::left)
            .def_readwrite("top", &LuaProxy::RECTd::top)
            .def_readwrite("right", &LuaProxy::RECTd::right)
            .def_readwrite("bottom", &LuaProxy::RECTd::bottom),

            LUAHELPER_DEF_CLASS_SMART_PTR_SHARED(Event, std::shared_ptr)
            .property("eventName", &Event::eventName)
            .property("cancellable", &Event::isCancellable)
            .property("cancelled", &Event::cancelled, &Event::setCancelled)
            .property("loopable", &Event::getLoopable, &Event::setLoopable)
            .property("directEventName", &Event::getDirectEventName, &Event::setDirectEventName),

            LUAHELPER_DEF_CLASS(Logger)
            .def(constructor<std::string>())
            .def("write", &LuaProxy::Logger::write),

            LUAHELPER_DEF_CLASS(Data)
                .enum_("DataTypes")
                [
                    value("DATA_LEVEL", LuaProxy::Data::DATA_LEVEL),
                    value("DATA_WORLD", LuaProxy::Data::DATA_WORLD),
                    value("DATA_GLOBAL", LuaProxy::Data::DATA_GLOBAL)
                ]
            .def(constructor<LuaProxy::Data::DataType>())
            .def(constructor<LuaProxy::Data::DataType, std::string>())
            .def(constructor<LuaProxy::Data::DataType, bool>())
            .def(constructor<LuaProxy::Data::DataType, std::string, bool>())
            .def("set", &LuaProxy::Data::set)
            .def("get", static_cast<std::string(LuaProxy::Data::*)(const std::string &) const>(&LuaProxy::Data::get))
            .def("get", static_cast<luabind::object(LuaProxy::Data::*)(lua_State*) const>(&LuaProxy::Data::get))
            .def("save", static_cast<void(LuaProxy::Data::*)()>(&LuaProxy::Data::save))
            .def("save", static_cast<void(LuaProxy::Data::*)(const std::string &)>(&LuaProxy::Data::save))
            .property("dataType", &LuaProxy::Data::dataType, &LuaProxy::Data::setDataType)
            .property("sectionName", &LuaProxy::Data::sectionName, &LuaProxy::Data::setSectionName)
            .property("useSaveSlot", &LuaProxy::Data::useSaveSlot, &LuaProxy::Data::setUseSaveSlot),

            LUAHELPER_DEF_CLASS(AsyncHTTPRequest)
            .enum_("HTTP_METHOD")[
                value("HTTP_POST", AsyncHTTPClient::HTTP_POST),
                value("HTTP_GET", AsyncHTTPClient::HTTP_GET)
            ]
            .def(constructor<>())
            .def("addArgument", &LuaProxy::AsyncHTTPRequest::addArgument)
            .def("send", &LuaProxy::AsyncHTTPRequest::send)
            .def("wait", &LuaProxy::AsyncHTTPRequest::wait)
            .property("url", &LuaProxy::AsyncHTTPRequest::getUrl, &LuaProxy::AsyncHTTPRequest::setUrl)
            .property("method", &LuaProxy::AsyncHTTPRequest::getMethod, &LuaProxy::AsyncHTTPRequest::setMethod)
            .property("ready", &LuaProxy::AsyncHTTPRequest::isReady)
            .property("processing", &LuaProxy::AsyncHTTPRequest::isProcessing)
            .property("finished", &LuaProxy::AsyncHTTPRequest::isFinished)
            .property("responseText", &LuaProxy::AsyncHTTPRequest::responseText)
			.property("responseBody", &LuaProxy::AsyncHTTPRequest::responseBody)
            .property("statusCode", &LuaProxy::AsyncHTTPRequest::statusCode),

            LUAHELPER_DEF_CLASS(PlayerSettings)
            .scope[
                def("get", &LuaProxy::PlayerSettings::get)
            ]
            .property("hitboxWidth", &LuaProxy::PlayerSettings::getHitboxWidth, &LuaProxy::PlayerSettings::setHitboxWidth)
            .property("hitboxHeight", &LuaProxy::PlayerSettings::getHitboxHeight, &LuaProxy::PlayerSettings::setHitboxHeight)
            .property("hitboxDuckHeight", &LuaProxy::PlayerSettings::getHitboxDuckHeight, &LuaProxy::PlayerSettings::setHitboxDuckHeight)
            .property("grabOffsetX", &LuaProxy::PlayerSettings::getGrabOffsetX, &LuaProxy::PlayerSettings::setGrabOffsetX)
            .property("grabOffsetY", &LuaProxy::PlayerSettings::getGrabOffsetY, &LuaProxy::PlayerSettings::setGrabOffsetY)
            .def("getSpriteOffsetX", &LuaProxy::PlayerSettings::getSpriteOffsetX)
            .def("setSpriteOffsetX", &LuaProxy::PlayerSettings::setSpriteOffsetX)
            .def("getSpriteOffsetY", &LuaProxy::PlayerSettings::getSpriteOffsetY)
            .def("setSpriteOffsetY", &LuaProxy::PlayerSettings::setSpriteOffsetY)


            .property("character", &LuaProxy::PlayerSettings::getCharacter, &LuaProxy::PlayerSettings::setCharacter)
            .property("powerup", &LuaProxy::PlayerSettings::getPowerupID, &LuaProxy::PlayerSettings::setPowerupID),


            LUAHELPER_DEF_CLASS(Player)
            .scope[ //static functions
                def("count", &LuaProxy::Player::count),
                    def("get", &LuaProxy::Player::get),
                    def("getTemplates", &LuaProxy::Player::getTemplates)
            ]
            .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::Player, m_index))

            .def(constructor<>())
            .def(constructor<int>())
            .def("mem", static_cast<void (LuaProxy::Player::*)(int, LuaProxy::L_FIELDTYPE, const luabind::object &, lua_State*)>(&LuaProxy::Player::mem))
            .def("mem", static_cast<luabind::object(LuaProxy::Player::*)(int, LuaProxy::L_FIELDTYPE, lua_State*) const>(&LuaProxy::Player::mem))
            .def("kill", &LuaProxy::Player::kill)
            .def("harm", &LuaProxy::Player::harm)
            .property("screen", &LuaProxy::Player::screen)
            .property("section", &LuaProxy::Player::section)
            .property("sectionObj", &LuaProxy::Player::sectionObj)
            .property("x", &LuaProxy::Player::x, &LuaProxy::Player::setX)
            .property("y", &LuaProxy::Player::y, &LuaProxy::Player::setY)
            .property("width", &LuaProxy::Player::width, &LuaProxy::Player::setWidth)
            .property("height", &LuaProxy::Player::height, &LuaProxy::Player::setHeight)
            .property("speedX", &LuaProxy::Player::speedX, &LuaProxy::Player::setSpeedX)
            .property("speedY", &LuaProxy::Player::speedY, &LuaProxy::Player::setSpeedY)
            .property("powerup", &LuaProxy::Player::powerup, &LuaProxy::Player::setPowerup)
            .property("character", &LuaProxy::Player::character, &LuaProxy::Player::setCharacter)
            .property("reservePowerup", &LuaProxy::Player::reservePowerup, &LuaProxy::Player::setReservePowerup)
            .property("holdingNPC", &LuaProxy::Player::holdingNPC)
            .property("upKeyPressing", &LuaProxy::Player::upKeyPressing, &LuaProxy::Player::setUpKeyPressing)
            .property("downKeyPressing", &LuaProxy::Player::downKeyPressing, &LuaProxy::Player::setDownKeyPressing)
            .property("leftKeyPressing", &LuaProxy::Player::leftKeyPressing, &LuaProxy::Player::setLeftKeyPressing)
            .property("rightKeyPressing", &LuaProxy::Player::rightKeyPressing, &LuaProxy::Player::setRightKeyPressing)
            .property("jumpKeyPressing", &LuaProxy::Player::jumpKeyPressing, &LuaProxy::Player::setJumpKeyPressing)
            .property("altJumpKeyPressing", &LuaProxy::Player::altJumpKeyPressing, &LuaProxy::Player::setAltJumpKeyPressing)
            .property("runKeyPressing", &LuaProxy::Player::runKeyPressing, &LuaProxy::Player::setRunKeyPressing)
            .property("altRunKeyPressing", &LuaProxy::Player::altRunKeyPressing, &LuaProxy::Player::setAltRunKeyPressing)
            .property("dropItemKeyPressing", &LuaProxy::Player::dropItemKeyPressing, &LuaProxy::Player::setDropItemKeyPressing)
            .property("pauseKeyPressing", &LuaProxy::Player::pauseKeyPressing, &LuaProxy::Player::setPauseKeyPressing)
            .def("getCurrentPlayerSetting", &LuaProxy::Player::getCurrentPlayerSetting)
            .def("getCurrentSpriteIndex", &LuaProxy::Player::getCurrentSpriteIndex, pure_out_value(_2) + pure_out_value(_3))
            .def("setCurrentSpriteIndex", &LuaProxy::Player::setCurrentSpriteIndex)
            .property("isValid", &LuaProxy::Player::isValid)
            /*Generated by code*/
#ifdef _MSC_VER //Generated by code
#pragma region
#endif
            .property("ToadDoubleJReady", &LuaProxy::Player::toadDoubleJReady, &LuaProxy::Player::setToadDoubleJReady)
            .property("SparklingEffect", &LuaProxy::Player::sparklingEffect, &LuaProxy::Player::setSparklingEffect)
            .property("UnknownCTRLLock1", &LuaProxy::Player::unknownCTRLLock1, &LuaProxy::Player::setUnknownCTRLLock1)
            .property("UnknownCTRLLock2", &LuaProxy::Player::unknownCTRLLock2, &LuaProxy::Player::setUnknownCTRLLock2)
            .property("QuicksandEffectTimer", &LuaProxy::Player::quicksandEffectTimer, &LuaProxy::Player::setQuicksandEffectTimer)
            .property("OnSlipperyGround", &LuaProxy::Player::onSlipperyGround, &LuaProxy::Player::setOnSlipperyGround)
            .property("IsAFairy", &LuaProxy::Player::isAFairy, &LuaProxy::Player::setIsAFairy)
            .property("FairyAlreadyInvoked", &LuaProxy::Player::fairyAlreadyInvoked, &LuaProxy::Player::setFairyAlreadyInvoked)
            .property("FairyFramesLeft", &LuaProxy::Player::fairyFramesLeft, &LuaProxy::Player::setFairyFramesLeft)
            .property("SheathHasKey", &LuaProxy::Player::sheathHasKey, &LuaProxy::Player::setSheathHasKey)
            .property("SheathAttackCooldown", &LuaProxy::Player::sheathAttackCooldown, &LuaProxy::Player::setSheathAttackCooldown)
            .property("Hearts", &LuaProxy::Player::hearts, &LuaProxy::Player::setHearts)
            .property("PeachHoverAvailable", &LuaProxy::Player::peachHoverAvailable, &LuaProxy::Player::setPeachHoverAvailable)
            .property("PressingHoverButton", &LuaProxy::Player::pressingHoverButton, &LuaProxy::Player::setPressingHoverButton)
            .property("PeachHoverTimer", &LuaProxy::Player::peachHoverTimer, &LuaProxy::Player::setPeachHoverTimer)
            .property("Unused1", &LuaProxy::Player::unused1, &LuaProxy::Player::setUnused1)
            .property("PeachHoverTrembleSpeed", &LuaProxy::Player::peachHoverTrembleSpeed, &LuaProxy::Player::setPeachHoverTrembleSpeed)
            .property("PeachHoverTrembleDir", &LuaProxy::Player::peachHoverTrembleDir, &LuaProxy::Player::setPeachHoverTrembleDir)
            .property("ItemPullupTimer", &LuaProxy::Player::itemPullupTimer, &LuaProxy::Player::setItemPullupTimer)
            .property("ItemPullupMomentumSave", &LuaProxy::Player::itemPullupMomentumSave, &LuaProxy::Player::setItemPullupMomentumSave)
            .property("Unused2", &LuaProxy::Player::unused2, &LuaProxy::Player::setUnused2)
            .property("UnkClimbing1", &LuaProxy::Player::unkClimbing1, &LuaProxy::Player::setUnkClimbing1)
            .property("UnkClimbing2", &LuaProxy::Player::unkClimbing2, &LuaProxy::Player::setUnkClimbing2)
            .property("UnkClimbing3", &LuaProxy::Player::unkClimbing3, &LuaProxy::Player::setUnkClimbing3)
            .property("WaterState", &LuaProxy::Player::waterOrQuicksandState, &LuaProxy::Player::setWaterOrQuicksandState) // Compat (this was undocumented auto-generated anyway)
            .property("WaterOrQuicksandState", &LuaProxy::Player::waterOrQuicksandState, &LuaProxy::Player::setWaterOrQuicksandState)
            .property("IsInWater", &LuaProxy::Player::isInWater, &LuaProxy::Player::setIsInWater)
            .property("WaterStrokeTimer", &LuaProxy::Player::waterStrokeTimer, &LuaProxy::Player::setWaterStrokeTimer)
            .property("UnknownHoverTimer", &LuaProxy::Player::unknownHoverTimer, &LuaProxy::Player::setUnknownHoverTimer)
            .property("SlidingState", &LuaProxy::Player::slidingState, &LuaProxy::Player::setSlidingState)
            .property("SlidingGroundPuffs", &LuaProxy::Player::slidingGroundPuffs, &LuaProxy::Player::setSlidingGroundPuffs)
            .property("ClimbingState", &LuaProxy::Player::climbingState, &LuaProxy::Player::setClimbingState)
            .property("UnknownTimer", &LuaProxy::Player::unknownTimer, &LuaProxy::Player::setUnknownTimer)
            .property("UnknownFlag", &LuaProxy::Player::unknownFlag, &LuaProxy::Player::setUnknownFlag)
            .property("UnknownPowerupState", &LuaProxy::Player::unknownPowerupState, &LuaProxy::Player::setUnknownPowerupState)
            .property("SlopeRelated", &LuaProxy::Player::slopeRelated, &LuaProxy::Player::setSlopeRelated)
            .property("TanookiStatueActive", &LuaProxy::Player::tanookiStatueActive, &LuaProxy::Player::setTanookiStatueActive)
            .property("TanookiMorphCooldown", &LuaProxy::Player::tanookiMorphCooldown, &LuaProxy::Player::setTanookiMorphCooldown)
            .property("TanookiActiveFrameCount", &LuaProxy::Player::tanookiActiveFrameCount, &LuaProxy::Player::setTanookiActiveFrameCount)
            .property("IsSpinjumping", &LuaProxy::Player::isSpinjumping, &LuaProxy::Player::setIsSpinjumping)
            .property("SpinjumpStateCounter", &LuaProxy::Player::spinjumpStateCounter, &LuaProxy::Player::setSpinjumpStateCounter)
            .property("SpinjumpLandDirection", &LuaProxy::Player::spinjumpLandDirection, &LuaProxy::Player::setSpinjumpLandDirection)
            .property("CurrentKillCombo", &LuaProxy::Player::currentKillCombo, &LuaProxy::Player::setCurrentKillCombo)
            .property("GroundSlidingPuffsState", &LuaProxy::Player::groundSlidingPuffsState, &LuaProxy::Player::setGroundSlidingPuffsState)
            .property("WarpNearby", &LuaProxy::Player::nearbyWarpIndex, &LuaProxy::Player::setNearbyWarpIndex) // Compat (this was undocumented auto-generated anyway)
            .property("NearbyWarpIndex", &LuaProxy::Player::nearbyWarpIndex, &LuaProxy::Player::setNearbyWarpIndex)
            .property("Unknown5C", &LuaProxy::Player::unknown5C, &LuaProxy::Player::setUnknown5C)
            .property("Unknown5E", &LuaProxy::Player::unknown5E, &LuaProxy::Player::setUnknown5E)
            .property("HasJumped", &LuaProxy::Player::hasJumped, &LuaProxy::Player::setHasJumped)
            .property("CurXPos", &LuaProxy::Player::curXPos, &LuaProxy::Player::setCurXPos)
            .property("CurYPos", &LuaProxy::Player::curYPos, &LuaProxy::Player::setCurYPos)
            .property("Height", &LuaProxy::Player::height, &LuaProxy::Player::setHeight)
            .property("Width", &LuaProxy::Player::width, &LuaProxy::Player::setWidth)
            .property("CurXSpeed", &LuaProxy::Player::curXSpeed, &LuaProxy::Player::setCurXSpeed)
            .property("CurYSpeed", &LuaProxy::Player::curYSpeed, &LuaProxy::Player::setCurYSpeed)
            .property("Identity", &LuaProxy::Player::identity, &LuaProxy::Player::setIdentity)
            .property("UKeyState", &LuaProxy::Player::uKeyState, &LuaProxy::Player::setUKeyState)
            .property("DKeyState", &LuaProxy::Player::dKeyState, &LuaProxy::Player::setDKeyState)
            .property("LKeyState", &LuaProxy::Player::lKeyState, &LuaProxy::Player::setLKeyState)
            .property("RKeyState", &LuaProxy::Player::rKeyState, &LuaProxy::Player::setRKeyState)
            .property("JKeyState", &LuaProxy::Player::jKeyState, &LuaProxy::Player::setJKeyState)
            .property("SJKeyState", &LuaProxy::Player::sJKeyState, &LuaProxy::Player::setSJKeyState)
            .property("XKeyState", &LuaProxy::Player::xKeyState, &LuaProxy::Player::setXKeyState)
            .property("RNKeyState", &LuaProxy::Player::rNKeyState, &LuaProxy::Player::setRNKeyState)
            .property("SELKeyState", &LuaProxy::Player::sELKeyState, &LuaProxy::Player::setSELKeyState)
            .property("STRKeyState", &LuaProxy::Player::sTRKeyState, &LuaProxy::Player::setSTRKeyState)
            .property("FacingDirection", &LuaProxy::Player::facingDirection, &LuaProxy::Player::setFacingDirection)
            .property("MountType", &LuaProxy::Player::mountType, &LuaProxy::Player::setMountType)
            .property("MountColor", &LuaProxy::Player::mountColor, &LuaProxy::Player::setMountColor)
            .property("MountState", &LuaProxy::Player::mountState, &LuaProxy::Player::setMountState)
            .property("MountHeightOffset", &LuaProxy::Player::mountHeightOffset, &LuaProxy::Player::setMountHeightOffset)
            .property("MountGfxIndex", &LuaProxy::Player::mountGfxIndex, &LuaProxy::Player::setMountGfxIndex)
            .property("CurrentPowerup", &LuaProxy::Player::currentPowerup, &LuaProxy::Player::setCurrentPowerup)
            .property("CurrentPlayerSprite", &LuaProxy::Player::currentPlayerSprite, &LuaProxy::Player::setCurrentPlayerSprite)
            .property("Unused116", &LuaProxy::Player::unused116, &LuaProxy::Player::setUnused116)
            .property("GfxMirrorX", &LuaProxy::Player::gfxMirrorX, &LuaProxy::Player::setGfxMirrorX)
            .property("UpwardJumpingForce", &LuaProxy::Player::upwardJumpingForce, &LuaProxy::Player::setUpwardJumpingForce)
            .property("JumpButtonHeld", &LuaProxy::Player::jumpButtonHeld, &LuaProxy::Player::setJumpButtonHeld)
            .property("SpinjumpButtonHeld", &LuaProxy::Player::spinjumpButtonHeld, &LuaProxy::Player::setSpinjumpButtonHeld)
            .property("ForcedAnimationState", &LuaProxy::Player::forcedAnimationState, &LuaProxy::Player::setForcedAnimationState)
            .property("ForcedAnimationTimer", &LuaProxy::Player::forcedAnimationTimer, &LuaProxy::Player::setForcedAnimationTimer)
            .property("DownButtonMirror", &LuaProxy::Player::downButtonMirror, &LuaProxy::Player::setDownButtonMirror)
            .property("InDuckingPosition", &LuaProxy::Player::inDuckingPosition, &LuaProxy::Player::setInDuckingPosition)
            .property("SelectButtonMirror", &LuaProxy::Player::selectButtonMirror, &LuaProxy::Player::setSelectButtonMirror)
            .property("Unknown132", &LuaProxy::Player::unknown132, &LuaProxy::Player::setUnknown132)
            .property("DownButtonTapped", &LuaProxy::Player::downButtonTapped, &LuaProxy::Player::setDownButtonTapped)
            .property("Unknown136", &LuaProxy::Player::unknown136, &LuaProxy::Player::setUnknown136)
            .property("XMomentumPush", &LuaProxy::Player::xMomentumPush, &LuaProxy::Player::setXMomentumPush)
            .property("DeathState", &LuaProxy::Player::deathState, &LuaProxy::Player::setDeathState)
            .property("DeathTimer", &LuaProxy::Player::deathTimer, &LuaProxy::Player::setDeathTimer)
            .property("BlinkTimer", &LuaProxy::Player::blinkTimer, &LuaProxy::Player::setBlinkTimer)
            .property("BlinkState", &LuaProxy::Player::blinkState, &LuaProxy::Player::setBlinkState)
            .property("Unknown144", &LuaProxy::Player::unknown144, &LuaProxy::Player::setUnknown144)
            .property("LayerStateStanding", &LuaProxy::Player::layerStateStanding, &LuaProxy::Player::setLayerStateStanding)
            .property("LayerStateLeftContact", &LuaProxy::Player::layerStateLeftContact, &LuaProxy::Player::setLayerStateLeftContact)
            .property("LayerStateTopContact", &LuaProxy::Player::layerStateTopContact, &LuaProxy::Player::setLayerStateTopContact)
            .property("LayerStateRightContact", &LuaProxy::Player::layerStateRightContact, &LuaProxy::Player::setLayerStateRightContact)
            .property("PushedByMovingLayer", &LuaProxy::Player::pushedByMovingLayer, &LuaProxy::Player::setPushedByMovingLayer)
            .property("Unused150", &LuaProxy::Player::unused150, &LuaProxy::Player::setUnused150)
            .property("Unused152", &LuaProxy::Player::unused152, &LuaProxy::Player::setUnused152)
            .property("HeldNPCIndex", &LuaProxy::Player::heldNPCIndex, &LuaProxy::Player::setHeldNPCIndex)
            .property("Unknown156", &LuaProxy::Player::unknown156, &LuaProxy::Player::setUnknown156)
            .property("PowerupBoxContents", &LuaProxy::Player::powerupBoxContents, &LuaProxy::Player::setPowerupBoxContents)
            .property("CurrentSection", &LuaProxy::Player::currentSection, &LuaProxy::Player::setCurrentSection)
            .property("WarpTimer", &LuaProxy::Player::warpCooldownTimer, &LuaProxy::Player::setWarpCooldownTimer) // Compat (this was undocumented auto-generated anyway)
            .property("WarpCooldownTimer", &LuaProxy::Player::warpCooldownTimer, &LuaProxy::Player::setWarpCooldownTimer)
            .property("TargetWarpIndex", &LuaProxy::Player::targetWarpIndex, &LuaProxy::Player::setTargetWarpIndex)
            .property("ProjectileTimer1", &LuaProxy::Player::projectileTimer1, &LuaProxy::Player::setProjectileTimer1)
            .property("ProjectileTimer2", &LuaProxy::Player::projectileTimer2, &LuaProxy::Player::setProjectileTimer2)
            .property("TailswipeTimer", &LuaProxy::Player::tailswipeTimer, &LuaProxy::Player::setTailswipeTimer)
            .property("Unknown166", &LuaProxy::Player::unknown166, &LuaProxy::Player::setUnknown166)
            .property("TakeoffSpeed", &LuaProxy::Player::takeoffSpeed, &LuaProxy::Player::setTakeoffSpeed)
            .property("CanFly", &LuaProxy::Player::canFly, &LuaProxy::Player::setCanFly)
            .property("IsFlying", &LuaProxy::Player::isFlying, &LuaProxy::Player::setIsFlying)
            .property("FlightTimeRemaining", &LuaProxy::Player::flightTimeRemaining, &LuaProxy::Player::setFlightTimeRemaining)
            .property("HoldingFlightRunButton", &LuaProxy::Player::holdingFlightRunButton, &LuaProxy::Player::setHoldingFlightRunButton)
            .property("HoldingFlightButton", &LuaProxy::Player::holdingFlightButton, &LuaProxy::Player::setHoldingFlightButton)
            .property("NPCBeingStoodOnIndex", &LuaProxy::Player::nPCBeingStoodOnIndex, &LuaProxy::Player::setNPCBeingStoodOnIndex)
            .property("Unknown178", &LuaProxy::Player::unknown178, &LuaProxy::Player::setUnknown178)
            .property("Unknown17A", &LuaProxy::Player::unknown17A, &LuaProxy::Player::setUnknown17A)
            .property("Unused17C", &LuaProxy::Player::unused17C, &LuaProxy::Player::setUnused17C)
            .property("Unused17E", &LuaProxy::Player::unused17E, &LuaProxy::Player::setUnused17E)
            .property("Unused180", &LuaProxy::Player::unused180, &LuaProxy::Player::setUnused180)
            .property("Unused182", &LuaProxy::Player::unused182, &LuaProxy::Player::setUnused182),
#ifdef _MSC_VER //Generated by code
#pragma endregion
#endif

            LUAHELPER_DEF_CLASS(Camera)
            .scope[ //static functions
                def("get", static_cast<luabind::object(*)(lua_State* L)>(&LuaProxy::Camera::get)),
                def("getX", static_cast<double(*)(unsigned short)>(&LuaProxy::Camera::getX)),
                def("getY", static_cast<double(*)(unsigned short)>(&LuaProxy::Camera::getY))
            ]
            .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::Camera, m_index))
            .def("mem", static_cast<void (LuaProxy::Camera::*)(int, LuaProxy::L_FIELDTYPE, const luabind::object &, lua_State*)>(&LuaProxy::Camera::mem))
            .def("mem", static_cast<luabind::object(LuaProxy::Camera::*)(int, LuaProxy::L_FIELDTYPE, lua_State*) const>(&LuaProxy::Camera::mem))
            .property("x", &LuaProxy::Camera::x, &LuaProxy::Camera::setX)
            .property("y", &LuaProxy::Camera::y, &LuaProxy::Camera::setY)
            .property("renderX", &LuaProxy::Camera::renderX, &LuaProxy::Camera::setRenderX)
            .property("renderY", &LuaProxy::Camera::renderY, &LuaProxy::Camera::setRenderY)
            .property("width", &LuaProxy::Camera::width, &LuaProxy::Camera::setWidth)
            .property("height", &LuaProxy::Camera::height, &LuaProxy::Camera::setHeight),


            def("newRECT", &LuaProxy::newRECT),
            def("newRECTd", &LuaProxy::newRECTd),

            namespace_("UserData")[
                def("setValue", &LuaProxy::SaveBankProxy::setValue),
                    def("getValue", &LuaProxy::SaveBankProxy::getValue),
                    def("isValueSet", &LuaProxy::SaveBankProxy::isValueSet),
                    def("values", &LuaProxy::SaveBankProxy::values),
                    def("save", &LuaProxy::SaveBankProxy::save)
            ],

            LUAHELPER_DEF_CLASS(VBStr)
            .def(constructor<long>())
            .property("str", &LuaProxy::VBStr::str, &LuaProxy::VBStr::setStr)
            .property("length", &LuaProxy::VBStr::length, &LuaProxy::VBStr::setLength)
            .def("clear", &LuaProxy::VBStr::clear)
            .def(tostring(self))
            .def("__concat", &LuaProxy::VBStr::luaConcat),

            class_<LuaProxy::Console>("Console")
            .def("print", &LuaProxy::Console::print)
            .def("println", &LuaProxy::Console::println)
        ];
    if(m_type == LUNALUA_WORLD){
        module(L)
            [
                namespace_("Graphics")[
                    def("activateOverworldHud", &LuaProxy::Graphics::activateOverworldHud),
                    def("getOverworldHudState", &LuaProxy::Graphics::getOverworldHudState)
                ],

                LUAHELPER_DEF_CLASS(World)
                .property("playerX", &LuaProxy::World::playerX, &LuaProxy::World::setPlayerX)
                .property("playerY", &LuaProxy::World::playerY, &LuaProxy::World::setPlayerY)
                .property("playerWalkingDirection", &LuaProxy::World::currentWalkingDirection, &LuaProxy::World::setCurrentWalkingDirection)
                .property("playerWalkingTimer", &LuaProxy::World::currentWalkingTimer, &LuaProxy::World::setCurrentWalkingTimer)
                .property("playerWalkingFrame", &LuaProxy::World::currentWalkingFrame, &LuaProxy::World::setCurrentWalkingFrame)    
                .property("playerWalkingFrameTimer", &LuaProxy::World::currentWalkingFrameTimer, &LuaProxy::World::setCurrentWalkingFrameTimer)
                .property("playerIsCurrentWalking", &LuaProxy::World::playerIsCurrentWalking)
                .property("levelTitle", &LuaProxy::World::levelTitle)
                .property("levelObj", &LuaProxy::World::levelObj)
                .property("playerCurrentDirection", &LuaProxy::World::getCurrentDirection)
                .property("playerPowerup", &LuaProxy::World::playerPowerup, &LuaProxy::World::setPlayerPowerup)
                .def("mem", static_cast<void (LuaProxy::World::*)(int, LuaProxy::L_FIELDTYPE, const luabind::object &, lua_State*)>(&LuaProxy::World::mem))
                .def("mem", static_cast<luabind::object(LuaProxy::World::*)(int, LuaProxy::L_FIELDTYPE, lua_State*) const>(&LuaProxy::World::mem)),

                LUAHELPER_DEF_CLASS(Tile)
                .scope[ //static functions
                    def("count", &LuaProxy::Tile::count),
                    def("get", static_cast<luabind::object(*)(lua_State* L)>(&LuaProxy::Tile::get)),
                    def("get", static_cast<luabind::object(*)(luabind::object, lua_State* L)>(&LuaProxy::Tile::get)),
                    def("getIntersecting", &LuaProxy::Tile::getIntersecting)
                ]
                .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::Tile, m_index))
                .def(constructor<int>())
                .property("id", &LuaProxy::Tile::id, &LuaProxy::Tile::setId)
                .property("x", &LuaProxy::Tile::x, &LuaProxy::Tile::setX)
                .property("y", &LuaProxy::Tile::y, &LuaProxy::Tile::setY)
                .property("width", &LuaProxy::Tile::width, &LuaProxy::Tile::setWidth)
                .property("height", &LuaProxy::Tile::height, &LuaProxy::Tile::setHeight)
                .property("isValid", &LuaProxy::Tile::isValid),

                LUAHELPER_DEF_CLASS(Scenery)
                .scope[ //static functions
                    def("count", &LuaProxy::Scenery::count),
                    def("get", static_cast<luabind::object(*)(lua_State* L)>(&LuaProxy::Scenery::get)),
                    def("get", static_cast<luabind::object(*)(luabind::object, lua_State* L)>(&LuaProxy::Scenery::get)),
                    def("getIntersecting", &LuaProxy::Scenery::getIntersecting)
                ]
                .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::Scenery, m_index))
                .def(constructor<int>())
                .property("id", &LuaProxy::Scenery::id, &LuaProxy::Scenery::setId)
                .property("x", &LuaProxy::Scenery::x, &LuaProxy::Scenery::setX)
                .property("y", &LuaProxy::Scenery::y, &LuaProxy::Scenery::setY)
                .property("width", &LuaProxy::Scenery::width, &LuaProxy::Scenery::setWidth)
                .property("height", &LuaProxy::Scenery::height, &LuaProxy::Scenery::setHeight)
                .property("isValid", &LuaProxy::Scenery::isValid),

                LUAHELPER_DEF_CLASS(Path)
                .scope[ //static functions
                    def("count", &LuaProxy::Path::count),
                    def("get", static_cast<luabind::object(*)(lua_State* L)>(&LuaProxy::Path::get)),
                    def("get", static_cast<luabind::object(*)(luabind::object, lua_State* L)>(&LuaProxy::Path::get)),
                    def("getIntersecting", &LuaProxy::Path::getIntersecting)
                ]
                .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::Path, m_index))
                .def(constructor<int>())
                .property("id", &LuaProxy::Path::id, &LuaProxy::Path::setId)
                .property("x", &LuaProxy::Path::x, &LuaProxy::Path::setX)
                .property("y", &LuaProxy::Path::y, &LuaProxy::Path::setY)
                .property("width", &LuaProxy::Path::width, &LuaProxy::Path::setWidth)
                .property("height", &LuaProxy::Path::height, &LuaProxy::Path::setHeight)
                .property("isValid", &LuaProxy::Path::isValid),

                LUAHELPER_DEF_CLASS(Musicbox)
                .scope[ //static functions
                    def("count", &LuaProxy::Musicbox::count),
                    def("get", static_cast<luabind::object(*)(lua_State* L)>(&LuaProxy::Musicbox::get)),
                    def("get", static_cast<luabind::object(*)(luabind::object, lua_State* L)>(&LuaProxy::Musicbox::get)),
                    def("getIntersecting", &LuaProxy::Musicbox::getIntersecting)
                ]
                .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::Musicbox, m_index))
                .def(constructor<int>())
                .property("id", &LuaProxy::Musicbox::id, &LuaProxy::Musicbox::setId)
                .property("x", &LuaProxy::Musicbox::x, &LuaProxy::Musicbox::setX)
                .property("y", &LuaProxy::Musicbox::y, &LuaProxy::Musicbox::setY)
                .property("width", &LuaProxy::Musicbox::width, &LuaProxy::Musicbox::setWidth)
                .property("height", &LuaProxy::Musicbox::height, &LuaProxy::Musicbox::setHeight)
                .property("isValid", &LuaProxy::Musicbox::isValid),

                LUAHELPER_DEF_CLASS(Level)
                .scope[ //static functions
                        def("count", &LuaProxy::LevelObject::count),
                        def("get", (luabind::object(*)(lua_State* L))&LuaProxy::LevelObject::get),
                        def("get", (luabind::object(*)(luabind::object, lua_State* L))&LuaProxy::LevelObject::get),
                        def("getByName", &LuaProxy::LevelObject::getByName),
                        def("getByFilename", &LuaProxy::LevelObject::getByFilename),
                        def("findByName", &LuaProxy::LevelObject::findByName),
                        def("findByFilename", &LuaProxy::LevelObject::findByFilename)
                ]
                .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::LevelObject, m_index))
                .property("x", &LuaProxy::LevelObject::x, &LuaProxy::LevelObject::setX)
                .property("y", &LuaProxy::LevelObject::y, &LuaProxy::LevelObject::setY)
                .property("goToX", &LuaProxy::LevelObject::goToX, &LuaProxy::LevelObject::setGoToX)
                .property("goToY", &LuaProxy::LevelObject::goToY, &LuaProxy::LevelObject::setGoToY)
                .property("topExitType", &LuaProxy::LevelObject::topExitType, &LuaProxy::LevelObject::setTopExitType)
                .property("leftExitType", &LuaProxy::LevelObject::leftExitType, &LuaProxy::LevelObject::setLeftExitType)
                .property("bottomExitType", &LuaProxy::LevelObject::bottomExitType, &LuaProxy::LevelObject::setBottomExitType)
                .property("rightExitType", &LuaProxy::LevelObject::rightExitType, &LuaProxy::LevelObject::setRightExitType)
                .property("levelWarpNumber", &LuaProxy::LevelObject::levelWarpNumber, &LuaProxy::LevelObject::setLevelWarpNumber)
                .property("isPathBackground", &LuaProxy::LevelObject::isPathBackground, &LuaProxy::LevelObject::setIsPathBackground)
                .property("isBigBackground", &LuaProxy::LevelObject::isBigBackground, &LuaProxy::LevelObject::setIsBigBackground)
                .property("isGameStartPoint", &LuaProxy::LevelObject::isGameStartPoint, &LuaProxy::LevelObject::setIsGameStartPoint)
                .property("isAlwaysVisible", &LuaProxy::LevelObject::isAlwaysVisible, &LuaProxy::LevelObject::setIsAlwaysVisible)
                .property("title", &LuaProxy::LevelObject::title)
                .property("filename", &LuaProxy::LevelObject::filename)
                .def("mem", static_cast<void (LuaProxy::LevelObject::*)(int, LuaProxy::L_FIELDTYPE, const luabind::object &, lua_State*)>(&LuaProxy::LevelObject::mem))
                .def("mem", static_cast<luabind::object(LuaProxy::LevelObject::*)(int, LuaProxy::L_FIELDTYPE, lua_State*) const>(&LuaProxy::LevelObject::mem))

            ];
    }

    if(m_type == LUNALUA_LEVEL){
        module(L)
            [
                namespace_("Text")[
                    def("showMessageBox", &LuaProxy::Text::showMessageBox)
                ],
                
                namespace_("Misc")[
                    def("npcToCoins", &LuaProxy::Misc::npcToCoins),
                    def("doPOW", &LuaProxy::Misc::doPOW),
                    def("doPSwitchRaw", &LuaProxy::Misc::doPSwitchRaw),
                    def("doPSwitch", (void(*)())&LuaProxy::Misc::doPSwitch),
                    def("doPSwitch", (void(*)(bool))&LuaProxy::Misc::doPSwitch),
                    def("doBombExplosion", (void(*)(double, double, short))&LuaProxy::Misc::doBombExplosion),
                    def("doBombExplosion", (void(*)(double, double, short, const LuaProxy::Player&))&LuaProxy::Misc::doBombExplosion)
                ],

                namespace_("Level")[
                    def("exit", &LuaProxy::Level::exit),
                    def("exitLevel", &LuaProxy::Level::exit), // Supposed to be 'exit' but one release had Level.exitLevel, so let that work too.
                    def("winState", (unsigned short(*)())&LuaProxy::Level::winState),
                    def("winState", (void(*)(unsigned short))&LuaProxy::Level::winState),
                    def("filename", &LuaProxy::Level::filename),
                    def("name", &LuaProxy::Level::name),
                    def("loadPlayerHitBoxes", (void(*)(int, int, const std::string&))&LuaProxy::loadHitboxes)
                ],

                namespace_("Graphics")[
                    def("activateHud", &LuaProxy::Graphics::activateHud),
                    def("isHudActivated", &LuaProxy::Graphics::isHudActivated)
                ],

                LUAHELPER_DEF_CLASS(Warp)
                .scope[
                        def("count", &LuaProxy::Warp::count),
                        def("get", &LuaProxy::Warp::get),
                        def("getIntersectingEntrance", &LuaProxy::Warp::getIntersectingEntrance),
                        def("getIntersectingExit", &LuaProxy::Warp::getIntersectingExit),
						def("spawn", static_cast<LuaProxy::Warp(*)(const luabind::object&, double, double, double, double, lua_State*)>(&LuaProxy::spawnWarp))
                ]
                .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::Warp, m_index))
                .def(constructor<int>())
                .def("mem", static_cast<void (LuaProxy::Warp::*)(int, LuaProxy::L_FIELDTYPE, const luabind::object &, lua_State*)>(&LuaProxy::Warp::mem))
                .def("mem", static_cast<luabind::object(LuaProxy::Warp::*)(int, LuaProxy::L_FIELDTYPE, lua_State*) const>(&LuaProxy::Warp::mem))
                .property("isHidden", &LuaProxy::Warp::isHidden, &LuaProxy::Warp::setIsHidden)
                .property("exitX", &LuaProxy::Warp::exitX, &LuaProxy::Warp::setExitX)
                .property("exitY", &LuaProxy::Warp::exitY, &LuaProxy::Warp::setExitY)
                .property("entranceX", &LuaProxy::Warp::entranceX, &LuaProxy::Warp::setEntranceX)
                .property("entranceY", &LuaProxy::Warp::entranceY, &LuaProxy::Warp::setEntranceY)
                .property("levelFilename", &LuaProxy::Warp::levelFilename, &LuaProxy::Warp::setLevelFilename),



                LUAHELPER_DEF_CLASS(Animation)
                .scope[ //static functions
                        def("count", &LuaProxy::Animation::count),
                        def("get", static_cast<luabind::object(*)(lua_State* L)>(&LuaProxy::Animation::get)),
                        def("get", static_cast<luabind::object(*)(luabind::object, lua_State* L)>(&LuaProxy::Animation::get)),
                        def("getIntersecting", &LuaProxy::Animation::getIntersecting),
                        def("spawn", (LuaProxy::Animation(*)(short, double, double, lua_State*))&LuaProxy::Animation::spawnEffect), //DONE
                        def("spawn", (LuaProxy::Animation(*)(short, double, double, float, lua_State*))&LuaProxy::Animation::spawnEffect) //DONE
                ]
                .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::Animation, m_animationIndex))
                .def(constructor<int>())
                .def("mem", static_cast<void (LuaProxy::Animation::*)(int, LuaProxy::L_FIELDTYPE, const luabind::object &, lua_State*)>(&LuaProxy::Animation::mem))
                .def("mem", static_cast<luabind::object (LuaProxy::Animation::*)(int, LuaProxy::L_FIELDTYPE, lua_State*) const>(&LuaProxy::Animation::mem))
                .property("id", &LuaProxy::Animation::id, &LuaProxy::Animation::setId)
                .property("x", &LuaProxy::Animation::x, &LuaProxy::Animation::setX)
                .property("y", &LuaProxy::Animation::y, &LuaProxy::Animation::setY)
                .property("speedX", &LuaProxy::Animation::speedX, &LuaProxy::Animation::setSpeedX)
                .property("speedY", &LuaProxy::Animation::speedY, &LuaProxy::Animation::setSpeedY)
                .property("width", &LuaProxy::Animation::width, &LuaProxy::Animation::setWidth)
                .property("height", &LuaProxy::Animation::height, &LuaProxy::Animation::setHeight)
                .property("subTimer", &LuaProxy::Animation::subTimer, &LuaProxy::Animation::setSubTimer)
                .property("timer", &LuaProxy::Animation::timer, &LuaProxy::Animation::setTimer)
                .property("animationFrame", &LuaProxy::Animation::animationFrame, &LuaProxy::Animation::setAnimationFrame)
                .property("npcID", &LuaProxy::Animation::npcID, &LuaProxy::Animation::setNpcID)
                .property("drawOnlyMask", &LuaProxy::Animation::drawOnlyMask, &LuaProxy::Animation::setDrawOnlyMask)
                .property("isValid", &LuaProxy::Animation::isValid),

                LUAHELPER_DEF_CLASS(Layer)
                .scope[ //static functions
                        def("get", (luabind::object(*)(lua_State* L))&LuaProxy::Layer::get),
                        def("get", (luabind::object(*)(const std::string& , lua_State* L))&LuaProxy::Layer::get),
                        def("find", &LuaProxy::Layer::find)
                ]
                .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::Layer, m_layerIndex))
                .def(constructor<int>())
                .def("stop", &LuaProxy::Layer::stop)
                .def("show", &LuaProxy::Layer::show)
                .def("hide", &LuaProxy::Layer::hide)
                .def("toggle", &LuaProxy::Layer::toggle)
                .property("isHidden", &LuaProxy::Layer::isHidden)
                .property("speedX", &LuaProxy::Layer::speedX, &LuaProxy::Layer::setSpeedX)
                .property("speedY", &LuaProxy::Layer::speedY, &LuaProxy::Layer::setSpeedY)
                .property("layerName", &LuaProxy::Layer::layerName),

                LUAHELPER_DEF_CLASS(Section)
                .scope[
                    def("get", (luabind::object(*)(lua_State* L))&LuaProxy::Section::get),
                    def("get", (LuaProxy::Section(*)(short, lua_State* L))&LuaProxy::Section::get)
                ]
                .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::Section, m_secNum))
                .def(constructor<int>())
                .property("boundary", &LuaProxy::Section::boundary, &LuaProxy::Section::setBoundary)
                .property("musicID", &LuaProxy::Section::musicID, &LuaProxy::Section::setMusicID)
                .property("isLevelWarp", &LuaProxy::Section::isLevelWarp, &LuaProxy::Section::setIsLevelWarp)
                .property("hasOffscreenExit", &LuaProxy::Section::hasOffscreenExit, &LuaProxy::Section::setHasOffscreenExit)
                .property("backgroundID", &LuaProxy::Section::backgroundID, &LuaProxy::Section::setBackgroundID)
                .property("noTurnBack", &LuaProxy::Section::noTurnBack, &LuaProxy::Section::setNoTurnBack)
                .property("isUnderwater", &LuaProxy::Section::isUnderwater, &LuaProxy::Section::setIsUnderwater),

                LUAHELPER_DEF_CLASS(NPC)
                .scope[ //static functions
                    def("count", &LuaProxy::NPC::count),
                    def("get", (luabind::object(*)(lua_State* L))&LuaProxy::NPC::get),
                    def("get", (luabind::object(*)(luabind::object, lua_State* L))&LuaProxy::NPC::get),
                    def("get", (luabind::object(*)(luabind::object, luabind::object, lua_State* L))&LuaProxy::NPC::get),
                    def("getIntersecting", &LuaProxy::NPC::getIntersecting),
                    def("spawn", static_cast<LuaProxy::NPC(*)(short, double, double, short, lua_State*)>(&LuaProxy::spawnNPC)),
                    def("spawn", static_cast<LuaProxy::NPC(*)(short, double, double, short, bool, lua_State*)>(&LuaProxy::spawnNPC)),
                    def("spawn", static_cast<LuaProxy::NPC(*)(short, double, double, short, bool, bool, lua_State*)>(&LuaProxy::spawnNPC))
                ]
                .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::NPC, m_index))
                .def(constructor<int>())
                .def("mem", static_cast<void (LuaProxy::NPC::*)(int, LuaProxy::L_FIELDTYPE, const luabind::object &, lua_State*)>(&LuaProxy::NPC::mem))
                .def("mem", static_cast<luabind::object(LuaProxy::NPC::*)(int, LuaProxy::L_FIELDTYPE, lua_State*) const>(&LuaProxy::NPC::mem))
                .def("kill", static_cast<void (LuaProxy::NPC::*)(lua_State*)>(&LuaProxy::NPC::kill))
                .def("kill", static_cast<void (LuaProxy::NPC::*)(int, lua_State*)>(&LuaProxy::NPC::kill))
                .def("toIce", &LuaProxy::NPC::toIce)
                .def("toCoin", &LuaProxy::NPC::toCoin)
                .def("harm", static_cast<void (LuaProxy::NPC::*)(lua_State*)>(&LuaProxy::NPC::harm))
                .def("harm", static_cast<void (LuaProxy::NPC::*)(short, lua_State*)>(&LuaProxy::NPC::harm))
                .def("harm", static_cast<void (LuaProxy::NPC::*)(short, float, lua_State*)>(&LuaProxy::NPC::harm))
                .property("idx", &LuaProxy::NPC::idx)
                .property("id", &LuaProxy::NPC::id, &LuaProxy::NPC::setId)
                .property("isHidden", &LuaProxy::NPC::isHidden, &LuaProxy::NPC::setIsHidden)
                .property("direction", &LuaProxy::NPC::direction, &LuaProxy::NPC::setDirection)
                .property("x", &LuaProxy::NPC::x, &LuaProxy::NPC::setX)
                .property("y", &LuaProxy::NPC::y, &LuaProxy::NPC::setY)
                .property("width", &LuaProxy::NPC::width, &LuaProxy::NPC::setWidth)
                .property("height", &LuaProxy::NPC::height, &LuaProxy::NPC::setHeight)
                .property("speedX", &LuaProxy::NPC::speedX, &LuaProxy::NPC::setSpeedX)
                .property("speedY", &LuaProxy::NPC::speedY, &LuaProxy::NPC::setSpeedY)
                .property("attachedLayerName", &LuaProxy::NPC::attachedLayerName, &LuaProxy::NPC::setAttachedLayerName)
                .property("activateEventName", &LuaProxy::NPC::activateEventName, &LuaProxy::NPC::setActivateEventName)
                .property("deathEventName", &LuaProxy::NPC::deathEventName, &LuaProxy::NPC::setDeathEventName)
                .property("noMoreObjInLayer", &LuaProxy::NPC::noMoreObjInLayer, &LuaProxy::NPC::setNoMoreObjInLayer)
                .property("talkEventName", &LuaProxy::NPC::talkEventName, &LuaProxy::NPC::setTalkEventName)
                .property("msg", &LuaProxy::NPC::msg, &LuaProxy::NPC::setMsg)
                .property("layerName", &LuaProxy::NPC::layerName, &LuaProxy::NPC::setLayerName)
                .property("attachedLayerObj", &LuaProxy::NPC::attachedLayerObj, &LuaProxy::NPC::setAttachedLayerObj)
                .property("layerObj", &LuaProxy::NPC::layerObj, &LuaProxy::NPC::setLayerObj)
                .property("ai1", &LuaProxy::NPC::ai1, &LuaProxy::NPC::setAi1)
                .property("ai2", &LuaProxy::NPC::ai2, &LuaProxy::NPC::setAi2)
                .property("ai3", &LuaProxy::NPC::ai3, &LuaProxy::NPC::setAi3)
                .property("ai4", &LuaProxy::NPC::ai4, &LuaProxy::NPC::setAi4)
                .property("ai5", &LuaProxy::NPC::ai5, &LuaProxy::NPC::setAi5)
                .property("drawOnlyMask", &LuaProxy::NPC::drawOnlyMask, &LuaProxy::NPC::setDrawOnlyMask)
                .property("invincibleToSword", &LuaProxy::NPC::isInvincibleToSword, &LuaProxy::NPC::setIsInvincibleToSword)
                .property("legacyBoss", &LuaProxy::NPC::legacyBoss, &LuaProxy::NPC::setLegacyBoss)
                .property("friendly", &LuaProxy::NPC::friendly, &LuaProxy::NPC::setFriendly)
                .property("dontMove", &LuaProxy::NPC::dontMove, &LuaProxy::NPC::setDontMove)
                .property("collidesBlockBottom", &LuaProxy::NPC::collidesBlockBottom, &LuaProxy::NPC::setCollidesBlockBottom)
                .property("collidesBlockLeft", &LuaProxy::NPC::collidesBlockLeft, &LuaProxy::NPC::setCollidesBlockLeft)
                .property("collidesBlockUp", &LuaProxy::NPC::collidesBlockUp, &LuaProxy::NPC::setCollidesBlockUp)
                .property("collidesBlockRight", &LuaProxy::NPC::collidesBlockRight, &LuaProxy::NPC::setCollidesBlockRight)
                .property("underwater", &LuaProxy::NPC::isUnderwater, &LuaProxy::NPC::setIsUnderwater)
                .property("animationFrame", &LuaProxy::NPC::animationFrame, &LuaProxy::NPC::setAnimationFrame)
                .property("animationTimer", &LuaProxy::NPC::animationTimer, &LuaProxy::NPC::setAnimationTimer)
                .property("killFlag", &LuaProxy::NPC::killFlag, &LuaProxy::NPC::setKillFlag)
                .property("isValid", &LuaProxy::NPC::isValid),


                LUAHELPER_DEF_CLASS(Block)
                .scope[ //static functions
                        def("count", &LuaProxy::Block::count),
                        def("get", (luabind::object(*)(lua_State* L))&LuaProxy::Block::get),
                        def("get", (luabind::object(*)(luabind::object, lua_State* L))&LuaProxy::Block::get),
                        def("getIntersecting", &LuaProxy::Block::getIntersecting),
                        def("spawn", &LuaProxy::Block::spawn)

                ]
                .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::Block, m_index))
                .def(constructor<int>())
                .def("mem", static_cast<void (LuaProxy::Block::*)(int, LuaProxy::L_FIELDTYPE, const luabind::object&, lua_State*)>(&LuaProxy::Block::mem))
                .def("mem", static_cast<luabind::object (LuaProxy::Block::*)(int, LuaProxy::L_FIELDTYPE, lua_State*) const>(&LuaProxy::Block::mem))
                .def("collidesWith", &LuaProxy::Block::collidesWith)
                .def("remove", static_cast<void (LuaProxy::Block::*)()>(&LuaProxy::Block::remove))
                .def("remove", static_cast<void (LuaProxy::Block::*)(bool)>(&LuaProxy::Block::remove))
                .def("hit", static_cast<void (LuaProxy::Block::*)()>(&LuaProxy::Block::hit))
                .def("hit", static_cast<void (LuaProxy::Block::*)(bool)>(&LuaProxy::Block::hit))
                .def("hit", static_cast<void (LuaProxy::Block::*)(bool, LuaProxy::Player)>(&LuaProxy::Block::hit))
                .def("hit", static_cast<void (LuaProxy::Block::*)(bool, LuaProxy::Player, int)>(&LuaProxy::Block::hit))
                .property("x", &LuaProxy::Block::x, &LuaProxy::Block::setX)
                .property("y", &LuaProxy::Block::y, &LuaProxy::Block::setY)
                .property("width", &LuaProxy::Block::width, &LuaProxy::Block::setWidth)
                .property("height", &LuaProxy::Block::height, &LuaProxy::Block::setHeight)
                .property("speedX", &LuaProxy::Block::speedX, &LuaProxy::Block::setSpeedX)
                .property("speedY", &LuaProxy::Block::speedY, &LuaProxy::Block::setSpeedY)
                .property("id", &LuaProxy::Block::id, &LuaProxy::Block::setId)
                .property("contentID", &LuaProxy::Block::contentID, &LuaProxy::Block::setContentID)
                .property("isHidden", &LuaProxy::Block::isHidden, &LuaProxy::Block::setIsHidden)
                .property("invisible", &LuaProxy::Block::isHidden, &LuaProxy::Block::setIsHidden)
                .property("slippery", &LuaProxy::Block::slippery, &LuaProxy::Block::setSlippery)
                .property("layerName", &LuaProxy::Block::layerName)
                .property("layerObj", &LuaProxy::Block::layerObj),

                LUAHELPER_DEF_CLASS(BGO)
                .scope[ //static functions
                        def("count", &LuaProxy::BGO::count),
                        def("get", static_cast<luabind::object(*)(lua_State* L)>(&LuaProxy::BGO::get)),
                        def("get", static_cast<luabind::object(*)(luabind::object, lua_State* L)>(&LuaProxy::BGO::get)),
                        def("getIntersecting", &LuaProxy::BGO::getIntersecting),
						def("spawn",&LuaProxy::spawnBGO)
                ]
                .def("__eq", LUAPROXY_DEFUSERDATAINEDXCOMPARE(LuaProxy::BGO, m_index))
                .def(constructor<int>())
                .property("id", &LuaProxy::BGO::id, &LuaProxy::BGO::setId)
                .property("isHidden", &LuaProxy::BGO::isHidden, &LuaProxy::BGO::setIsHidden)
                .property("x", &LuaProxy::BGO::x, &LuaProxy::BGO::setX)
                .property("y", &LuaProxy::BGO::y, &LuaProxy::BGO::setY)
                .property("width", &LuaProxy::BGO::width, &LuaProxy::BGO::setWidth)
                .property("height", &LuaProxy::BGO::height, &LuaProxy::BGO::setHeight)
                .property("speedX", &LuaProxy::BGO::speedX, &LuaProxy::BGO::setSpeedX)
                .property("speedY", &LuaProxy::BGO::speedY, &LuaProxy::BGO::setSpeedY)
                .property("layerName", &LuaProxy::BGO::layerName, &LuaProxy::BGO::setLayerName)
                .property("layer", &LuaProxy::BGO::layer, &LuaProxy::BGO::setLayer)
                .property("isValid", &LuaProxy::BGO::isValid)
            ];
    }
}


void CLunaLua::bindAllDeprecated()
{
    module(L)
        [
            def("getSMBXPath", &LuaProxy::Native::getSMBXPath), //DONE
			def("getWorldPath", &LuaProxy::Native::getWorldPath),
            def("simulateError", &LuaProxy::Native::simulateError), //DONE
            def("windowDebug", &LuaProxy::Text::windowDebug), //DONE
            def("printText", (void(*)(const luabind::object&, int, int)) &LuaProxy::Text::print), //DONE
            def("printText", (void(*)(const luabind::object&, int, int, int)) &LuaProxy::Text::print), //DONE
            def("loadImage", (bool(*)(const std::string&, int, int))&LuaProxy::Graphics::loadImage), //DONE
            def("placeSprite", (void(*)(int, int, int, int, const std::string&, int))&LuaProxy::Graphics::placeSprite), //DONE
            def("placeSprite", (void(*)(int, int, int, int, const std::string&))&LuaProxy::Graphics::placeSprite), //DONE
            def("placeSprite", (void(*)(int, int, int, int))&LuaProxy::Graphics::placeSprite), //DONE

            /*************************Audio*****************************/
            //Old Audio stuff
            def("playSFX", (void(*)(int))&LuaProxy::playSFX),
            def("playSFX", (void(*)(const std::string&))&LuaProxy::playSFX),
            def("playSFXSDL", (void(*)(const std::string&))&LuaProxy::playSFXSDL),
            def("clearSFXBuffer", (void(*)())&LuaProxy::clearSFXBuffer),
            def("MusicOpen", (void(*)(const std::string&))&LuaProxy::MusicOpen),
            def("MusicPlay", (void(*)())&LuaProxy::MusicPlay),
            def("MusicPlayFadeIn", (void(*)(int))&LuaProxy::MusicPlay),
            def("MusicStop", (void(*)())&LuaProxy::MusicStop),
            def("MusicStopFadeOut", (void(*)(int))&LuaProxy::MusicStopFadeOut),
            def("MusicVolume", (void(*)(int))&LuaProxy::MusicVolume)
        ];
    if (m_type == LUNALUA_WORLD){
        module(L)
            [
                def("levels", &LuaProxy::levels),
                def("findlevels", &LuaProxy::findlevels),
                def("findlevel", &LuaProxy::findlevel)
            ];
    }
    if (m_type == LUNALUA_LEVEL){
        module(L)
            [
                def("showMessageBox", &LuaProxy::Text::showMessageBox), //DONE
                def("totalNPC", &LuaProxy::totalNPCs), //DONE
                def("npcs", &LuaProxy::npcs), //DONE
                def("findnpcs", &LuaProxy::findNPCs), //New version working = DONE
                def("triggerEvent", &LuaProxy::triggerEvent), //In next version event namespace
                def("playMusic", &LuaProxy::playMusic), //DONE
                def("loadHitboxes", (void(*)(int, int, const std::string&))&LuaProxy::loadHitboxes),
                def("gravity", (unsigned short(*)())&LuaProxy::gravity), //DONE [DEPRECATED]
                def("gravity", (void(*)(unsigned short))&LuaProxy::gravity), //DONE [DEPRECATED]
                def("earthquake", (unsigned short(*)())&LuaProxy::earthquake), //DONE [DEPRECATED]
                def("earthquake", (void(*)(unsigned short))&LuaProxy::earthquake), //DONE [DEPRECATED]
                def("jumpheight", (unsigned short(*)())&LuaProxy::jumpheight), //DONE [DEPRECATED]
                def("jumpheight", (void(*)(unsigned short))&LuaProxy::jumpheight), //DONE [DEPRECATED]
                def("jumpheightBounce", (unsigned short(*)())&LuaProxy::jumpheightBounce), //DONE [DEPRECATED]
                def("jumpheightBounce", (void(*)(unsigned short))&LuaProxy::jumpheightBounce), //DONE [DEPRECATED]
                def("runAnimation", (void(*)(int, double, double, double, double, double, double, int))&LuaProxy::runAnimation), //DONE [DEPRECATED]
                def("runAnimation", (void(*)(int, double, double, double, double, int))&LuaProxy::runAnimation), //DONE [DEPRECATED]
                def("runAnimation", (void(*)(int, double, double, int))&LuaProxy::runAnimation), //DONE
                def("npcToCoins", &LuaProxy::Misc::npcToCoins), //DONE
                def("blocks", &LuaProxy::blocks),   //DONE
                def("findblocks", &LuaProxy::findblocks),   //DONE
                def("findlayer", &LuaProxy::findlayer),     //DONE
                def("exitLevel", &LuaProxy::Level::exit), //DONE
                def("winState", (unsigned short(*)())&LuaProxy::Level::winState), //DONE
                def("winState", (void(*)(unsigned short))&LuaProxy::Level::winState), //DONE
                def("animations", &LuaProxy::animations), //DONE
                def("getInput", &LuaProxy::getInput), //DONE
                def("hud", &LuaProxy::Graphics::activateHud), //DONE
                def("getLevelFilename", &LuaProxy::Level::filename), //DONE
                def("getLevelName", &LuaProxy::Level::name), //DONE
                def("spawnNPC", static_cast<LuaProxy::NPC(*)(short, double, double, short, lua_State*)>(&LuaProxy::spawnNPC)), //DONE
                def("spawnNPC", static_cast<LuaProxy::NPC(*)(short, double, double, short, bool, lua_State*)>(&LuaProxy::spawnNPC)), //DONE
                def("spawnNPC", static_cast<LuaProxy::NPC(*)(short, double, double, short, bool, bool, lua_State*)>(&LuaProxy::spawnNPC)), //DONE
                def("spawnEffect", (LuaProxy::Animation(*)(short, double, double, lua_State*))&LuaProxy::spawnEffect), //DONE
                def("spawnEffect", (LuaProxy::Animation(*)(short, double, double, float, lua_State*))&LuaProxy::spawnEffect) //DONE
            ];

    }
}


void CLunaLua::doEvents()
{
    //If the lua module is not valid anyway, then just return
    if(!isValid())
        return;

    // If is not ready (SMBX not init), then skip event loop
    if (!m_ready)
        return;

    
    //If player is not valid then shutdown the lua module
    if(m_type == LUNALUA_LEVEL){
        if(!Player::Get(1)){
            shutdown();
            return;
        }
    }

    MusicManager::setCurrentSection(Player::Get(1)->CurrentSection);

    if (!m_eventLoopOnceExecuted) {
        std::shared_ptr<Event> onStartEvent = std::make_shared<Event>("onStart", false);
        onStartEvent->setLoopable(false);
        onStartEvent->setDirectEventName("onStart");
        callEvent(onStartEvent);
        m_eventLoopOnceExecuted = true;
    
        // If an error happened in onStart then return.
        if (!isValid())
            return;
    }

    std::shared_ptr<Event> onLoopEvent = std::make_shared<Event>("onLoop", false);
    onLoopEvent->setLoopable(false);
    onLoopEvent->setDirectEventName("onLoop");
    callEvent(onLoopEvent);
    
    callLuaFunction(L, "__doEventQueue");
}

void CLunaLua::checkWarnings()
{
    if (m_warningList.size() > 0)
    {
        std::wostringstream message;
        message << L"Warnings occured during run:\r\n";
        for (auto iter = m_warningList.cbegin(), end = m_warningList.cend(); iter != end; ++iter)
        {
            message << L" - " << Str2WStr(*iter) << L"\r\n";
        }
        MessageBoxW(NULL, message.str().c_str(), L"LunaLua Warnings", MB_OK | MB_ICONWARNING);
    }

    m_warningList.clear();
}

void CLunaLua::setWarning(const std::string& str)
{
    // Only queue warnings if in editor
    if (GM_CUR_SAVE_SLOT == 0)
    {
        m_warningList.push_back(str);
    }
}
