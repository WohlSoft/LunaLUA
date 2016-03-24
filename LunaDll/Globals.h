//*** Globals.h - It has some globals in it ***
#ifndef globals_hhhhhh
#define globals_hhhhhh

#ifndef __MINGW32__
#pragma warning(disable: 4731)
#pragma warning(disable: 4800)
#pragma warning(disable: 4996) //localtime
#endif
#include "LevelCodes/LevelCodes.h"
#include "Rendering/Rendering.h"
#include "DeathCounter/DeathCounter.h"
#include "Autocode/AutocodeManager.h"
#include "Logging/Logging.h"
#include "CustomSprites/CSpriteManager.h"
#include "CellManager/CellManager.h"
#include "UserSaves/UserSaving.h"
#include "MciEmulator/mciEmulator.h"
#include "HardcodedGraphics/HardcodedGraphicsManager.h"
#include "Misc/SHMemServer.h"
#include "LuaMain/LunaLuaMain.h"
#include "GameConfig/GameAutostart.h"
#include "Misc/FreeImageUtils/FreeImageInit.h"
#include "GameConfig/GeneralLunaConfig.h"
#include "EventStateMachine.h"
/*
#include "Rendering/FFmpeg/FFmpeg.h"
*/

class RenderOverrideManager;

#include "Defines.h"

#ifndef __MINGW32__
#pragma comment (lib, "Msimg32.lib")
#endif

struct startUpSettings{
	bool patch;
	bool game;
	bool lvlEditor;
	bool frameskip;
	bool noSound;
	bool debugger;
	bool logger;
	bool newLauncher;
    bool console;
    bool noGL;
};

struct SMBXHUDSettings {
    bool              skip; // Skip the whole HUD drawing
    WORLD_HUD_CONTROL overworldHudState;
    bool              skipStarCount;
};

extern HINSTANCE	gHInstance;

/// Global settings
extern bool            gLunaEnabled;
extern bool            gShowDemoCounter;
extern bool            gPrintErrorsToScreen;
extern bool            gIsOverworld;
extern bool            gHook_SkipTestMsgBox;
extern bool            gAutostartRan;
extern bool            gIsWindowsVistaOrNewer;
extern SMBXHUDSettings gSMBXHUDSettings;

extern startUpSettings gStartupSettings;

/// General use globals - These are all defined in Globals.cpp ///
extern int		gFrames;

extern int		gLastDownPress;
extern int		gDownTapped;
extern int		gLastUpPress;
extern int		gUpTapped;
extern int		gLastLeftPress;
extern int		gLeftTapped;
extern int		gLastRightPress;
extern int		gRightTapped;

extern int		gLastJumpPress;
extern int		gJumpTapped;
extern int		gLastRunPress;
extern int		gRunTapped;

extern HDC		ghMemDC;		// General-use screen-compatible DC
extern HBITMAP	ghGeneralDIB;	// General-use screen-sized DIB
extern DWORD*	gpScreenBits;	// Pointer to screen bits for general use DIB

// Key status
extern short	gNumpad4;
extern short	gNumpad8;
extern short	gNumpad2;
extern short	gNumpad6;

// Level init
extern LvlEnum	gLevelEnum;

// Misc
extern Renderer				gLunaRender;
extern DeathCounter			gDeathCounter;
extern AutocodeManager		gAutoMan;
extern CSpriteManager		gSpriteMan;
extern Logger				gLogger;
extern CellManager			gCellMan;
extern SavedVariableBank	gSavedVarBank;
extern MciEmulator			gMciEmulator;
extern HardcodedGraphicsManager gGraphManager;
extern SHMemServer			gShMemServer;
extern RenderOverrideManager gRenderOverride;
extern FreeImageInit        gFreeImgInit;
extern GeneralLunaConfig    gGeneralConfig;

extern CLunaLua				gLunaLua;

/// HELPER MACROS ///
/*
if(!hRunProc){
std::string errMsg = "Failed to load 'run' in the Launcher dll D:!\nIs Lunadll.dll or LunadllNewLauncher.dll different versions?\nError code:";
errMsg += std::to_string((long long)GetLastError());
MessageBoxA(NULL, errMsg.c_str(), "Error", NULL);
FreeLibrary(newLauncherLib);
newLauncherLib = NULL;
return;
}
*/
#define GL_PROC_CHECK(procHandle, procName, moduleHandle, moduleName) \
	if(!procHandle){\
		std::string errMsg = "Failed to load 'procName' in moduleName D:!\nIs Lunadll.dll or moduleName different versions?\nError code:";\
		errMsg += std::to_string((long long)GetLastError());\
        MessageBoxA(NULL, errMsg.c_str(), "Error", 0);\
		FreeLibrary(moduleHandle);\
		moduleHandle = NULL;\
		return;\
	}


#endif
