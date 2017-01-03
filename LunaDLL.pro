TEMPLATE = lib
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += dll
CONFIG += static
CONFIG += c++11

TARGET = LunaDLL

QMAKE_CXXFLAGS += -Wno-unused-variable -Wno-unused-parameter -Wno-write-strings -mwindows

DEFINES += LunaDLL_LIBRARY LunaDLL_EXPORTS
DEFINES += DX_EXPORT=Q_DECL_EXPORT _USRDLL LUNADLL_EXPORTS

INCLUDEPATH += $$PWD/LunaDll
INCLUDEPATH += $$PWD/LunaDll/libs/boost/
INCLUDEPATH += $$PWD/LunaDll/libs/lua_mingw/include
INCLUDEPATH += $$PWD/LunaDll/libs/luabind-include
INCLUDEPATH += $$PWD/LunaDll/libs/sdl/include
INCLUDEPATH += $$PWD/LunaDll/libs/glew/include
INCLUDEPATH += $$PWD/LunaDll/libs/glew/include
INCLUDEPATH += $$PWD/LunaDll/libs/freeimage
LIBS += -L$$PWD/LunaDll/libs/lua_mingw/lib
LIBS += -L$$PWD/LunaDll/libs/sdl_mingw/lib
LIBS += -static -lkernel32 -static -luser32 -static -lgdi32 -static -lcomdlg32 -static -lmsimg32 #-static -lcomsuppw
LIBS += -static -lmsimg32 -static -ldsound -static -lwinspool -static -ladvapi32 -static -lole32 -static -loleaut32 -static -llua5.1
LIBS += -static -lwinmm -static -lSDL2main -lSDL2.dll -static -lSDL2_mixer.dll libversion -lDbghelp -lvorbisfile -lvorbis -lmad -lmikmod.dll -lflac -logg

SOURCES += \
    LunaDll/Autocode/Commands/AC_HeartSystem.cpp \
    LunaDll/Autocode/Commands/AC_LunaControl.cpp \
    LunaDll/Autocode/Commands/EnumCmd.cpp \
    LunaDll/Autocode/Commands/GenComp.cpp \
    LunaDll/Autocode/Autocode.cpp \
    LunaDll/Autocode/AutocodeManager.cpp \
    LunaDll/CellManager/CellManager.cpp \
    LunaDll/CustomSprites/Hitbox/Hitbox.cpp \
    LunaDll/CustomSprites/SpritesFuncs/SpriteBehaviorFuncs.cpp \
    LunaDll/CustomSprites/SpritesFuncs/SpriteDrawFuncs.cpp \
    LunaDll/CustomSprites/CSprite.cpp \
    LunaDll/CustomSprites/CSpriteManager.cpp \
    LunaDll/CustomSprites/SpriteComponent.cpp \
    LunaDll/DeathCounter/DeathCounter.cpp \
    LunaDll/DeathCounter/DeathRecord.cpp \
    LunaDll/GameConfig/GameAutostart.cpp \
    LunaDll/GameConfig/GameConfiguration.cpp \
    LunaDll/GameConfig/GeneralLunaConfig.cpp \
    LunaDll/HardcodedGraphics/HardcodedGraphicsManager.cpp \
    LunaDll/HardcodedGraphics/HardocodeGFXMap.cpp \
    LunaDll/Input/Input.cpp \
    LunaDll/IPC/IPCPipeServer.cpp \
    LunaDll/LevelCodes/dlltestlvlCode.cpp \
    LunaDll/LevelCodes/Docopoper-AbstractAssault.lvl.cpp \
    LunaDll/LevelCodes/Docopoper-Calleoca.cpp \
    LunaDll/LevelCodes/Docopoper-TheFloorisLava.lvl..cpp \
    LunaDll/LevelCodes/EuroShellRandD.lvl.cpp \
    LunaDll/LevelCodes/JosephStaleknight-CurtainTortoise.lvl.cpp \
    LunaDll/LevelCodes/Kil-DemosBrain.cpp \
    LunaDll/LevelCodes/KilArmoryCode.cpp \
    LunaDll/LevelCodes/SAJewers-QraestoliaCaverns.lvl.cpp \
    LunaDll/LevelCodes/SAJewers-Snowboardin.cpp \
    LunaDll/LevelCodes/Talkhaus-Science_Final_Battle.cpp \
    LunaDll/libs/ini-reader/INIReader.cpp \
    LunaDll/libs/luabind-src/class.cpp \
    LunaDll/libs/luabind-src/class_info.cpp \
    LunaDll/libs/luabind-src/class_registry.cpp \
    LunaDll/libs/luabind-src/class_rep.cpp \
    LunaDll/libs/luabind-src/create_class.cpp \
    LunaDll/libs/luabind-src/error.cpp \
    LunaDll/libs/luabind-src/exception_handler.cpp \
    LunaDll/libs/luabind-src/function.cpp \
    LunaDll/libs/luabind-src/inheritance.cpp \
    LunaDll/libs/luabind-src/link_compatibility.cpp \
    LunaDll/libs/luabind-src/object_rep.cpp \
    LunaDll/libs/luabind-src/open.cpp \
    LunaDll/libs/luabind-src/pcall.cpp \
    LunaDll/libs/luabind-src/scope.cpp \
    LunaDll/libs/luabind-src/stack_content_by_name.cpp \
    LunaDll/libs/luabind-src/weak_ref.cpp \
    LunaDll/libs/luabind-src/wrapper_base.cpp \
    LunaDll/libs/luasocket/luasocket.cpp \
    LunaDll/libs/luasocket/mime.cpp \
    LunaDll/libs/stackwalker/StackWalker.cpp \
    LunaDll/Logging/Logger.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyGlobalFunctions/LuaProxyGlobalFuncEffects.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyGlobalFunctions/LuaProxyGlobalFuncGraphics.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyGlobalFunctions/LuaProxyGlobalFuncLevel.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyGlobalFunctions/LuaProxyGlobalFuncMisc.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyGlobalFunctions/LuaProxyGlobalFuncNative.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyGlobalFunctions/LuaProxyGlobalFuncText.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyAnimation.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyAsyncHTTPRequest.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyAudio.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyBGO.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyBlock.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyCameraInfo.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyConsole.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyData.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyGlobalFunctions.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyLayer.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyLogger.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyMusicbox.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyNativeInput.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyNPC.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyPath.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyPlayer.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyPlayerSettings.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxySaveBank.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyScenery.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxySection.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyShader.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyTile.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyVBStr.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyWarp.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyWorld.cpp \
    LunaDll/LuaMain/LuaProxyComponent/LuaProxyWorldLevel.cpp \
    LunaDll/LuaMain/LuaHelper.cpp \
    LunaDll/LuaMain/LuaProxy.cpp \
    LunaDll/LuaMain/LuaProxyFFI.cpp \
    LunaDll/LuaMain/LuaSharedProxy.cpp \
    LunaDll/LuaMain/LunaLuaMain.cpp \
    LunaDll/MciEmulator/mciEmulator.cpp \
    LunaDll/Minigames/CGUI/CGUIContainer.cpp \
    LunaDll/Minigames/GameboyRPG.cpp \
    LunaDll/Minigames/Minigames.cpp \
    LunaDll/Misc/FreeImageUtils/FreeImageData.cpp \
    LunaDll/Misc/FreeImageUtils/FreeImageGifData.cpp \
    LunaDll/Misc/FreeImageUtils/FreeImageHelper.cpp \
    LunaDll/Misc/FreeImageUtils/FreeImageInit.cpp \
    LunaDll/Misc/Gui/GuiCrashNotify.cpp \
    LunaDll/Misc/Gui/RichTextDialog.cpp \
    LunaDll/Misc/MemoryScanner/MemoryScanner.cpp \
    LunaDll/Misc/RuntimeHookComponents/RuntimeHookCharacterId.cpp \
    LunaDll/Misc/RuntimeHookComponents/RuntimeHookFixups.cpp \
    LunaDll/Misc/RuntimeHookComponents/RuntimeHookGeneral.cpp \
    LunaDll/Misc/RuntimeHookComponents/RuntimeHookHooks.cpp \
    LunaDll/Misc/RuntimeHookComponents/RuntimeHookPublicFunctions.cpp \
    LunaDll/Misc/RuntimeHookManagers/LevelHUDController.cpp \
    LunaDll/Misc/RuntimeHookUtils/APIHook.cpp \
    LunaDll/Misc/AsyncHTTPClient.cpp \
    LunaDll/Misc/ErrorReporter.cpp \
    LunaDll/Misc/MiscFuncs.cpp \
    LunaDll/Misc/PerfTracker.cpp \
    LunaDll/Misc/PGEEditorCmdSender.cpp \
    LunaDll/Misc/Playground.cpp \
    LunaDll/Misc/RuntimeHook.cpp \
    LunaDll/Misc/SafeFPUControl.cpp \
    LunaDll/Misc/SHMemServer.cpp \
    LunaDll/Misc/TestMode.cpp \
    LunaDll/Misc/TestModeMenu.cpp \
    LunaDll/Misc/TypeLib.cpp \
    LunaDll/Misc/UniPath.cpp \
    LunaDll/Misc/VariantHelper.cpp \
    LunaDll/Misc/VB6StrPtr.cpp \
    LunaDll/Misc/WaitForTickEnd.cpp \
    LunaDll/Misc/win32_Unicode.cpp \
    LunaDll/Rendering/GL/GLContextManager.cpp \
    LunaDll/Rendering/GL/GLDraw.cpp \
    LunaDll/Rendering/GL/GLEngine.cpp \
    LunaDll/Rendering/GL/GLEngineCmds.cpp \
    LunaDll/Rendering/GL/GLEngineProxy.cpp \
    LunaDll/Rendering/GL/GLFramebuffer.cpp \
    LunaDll/Rendering/GL/GLInitTest.cpp \
    LunaDll/Rendering/GL/GLSplitSprite.cpp \
    LunaDll/Rendering/GL/GLSprite.cpp \
    LunaDll/Rendering/GL/GLTextureStore.cpp \
    LunaDll/Rendering/RenderOps/RenderBitmapOp.cpp \
    LunaDll/Rendering/RenderOps/RenderEffectOp.cpp \
    LunaDll/Rendering/RenderOps/RenderGLOp.cpp \
    LunaDll/Rendering/RenderOps/RenderRectOp.cpp \
    LunaDll/Rendering/RenderOps/RenderSpriteOp.cpp \
    LunaDll/Rendering/Shaders/GLShader.cpp \
    LunaDll/Rendering/Shaders/GLShaderAttributeInfo.cpp \
    LunaDll/Rendering/Shaders/GLShaderUniformInfo.cpp \
    LunaDll/Rendering/Shaders/GLShaderVariableEntry.cpp \
    LunaDll/Rendering/Shaders/GLShaderVariableInfo.cpp \
    LunaDll/Rendering/AsyncGifRecorder.cpp \
    LunaDll/Rendering/BitBltEmulation.cpp \
    LunaDll/Rendering/BMPBox.cpp \
    LunaDll/Rendering/FrameCapture.cpp \
    LunaDll/Rendering/Rendering.cpp \
    LunaDll/Rendering/RenderOverrideManager.cpp \
    LunaDll/Rendering/RenderUtils.cpp \
    LunaDll/Rendering/SMBXMaskedImage.cpp \
    LunaDll/SdlMusic/MusicDefList.cpp \
    LunaDll/SdlMusic/MusicManager.cpp \
    LunaDll/SdlMusic/SdlMusPlayer.cpp \
    LunaDll/SMBXInternal/Reconstructed/Util/NpcToCoins.cpp \
    LunaDll/SMBXInternal/Animation.cpp \
    LunaDll/SMBXInternal/Blocks.cpp \
    LunaDll/SMBXInternal/CustomGraphics.cpp \
    LunaDll/SMBXInternal/HardcodedGraphicsAccess.cpp \
    LunaDll/SMBXInternal/Layer.cpp \
    LunaDll/SMBXInternal/Level.cpp \
    LunaDll/SMBXInternal/NPCs.cpp \
    LunaDll/SMBXInternal/Overworld.cpp \
    LunaDll/SMBXInternal/PlayerMOB.cpp \
    LunaDll/SMBXInternal/SMBXEvents.cpp \
    LunaDll/SMBXInternal/Sound.cpp \
    LunaDll/SMBXInternal/WorldLevel.cpp \
    LunaDll/UserSaves/UserSaving.cpp \
    LunaDll/EventStateMachine.cpp \
    LunaDll/GlobalFuncs.cpp \
    LunaDll/Globals.cpp \
    LunaDll/Main.cpp \
    LunaDll/PerformanceTimer.cpp \
    LunaDll/PngRender.cpp \
    LunaDll/SMBXEvents.cpp \
    LunaDll/libs/ini-reader/ini.c \
    LunaDll/libs/luasocket/auxiliar.c \
    LunaDll/libs/luasocket/buffer.c \
    LunaDll/libs/luasocket/except.c \
    LunaDll/libs/luasocket/inet.c \
    LunaDll/libs/luasocket/io.c \
    LunaDll/libs/luasocket/options.c \
    LunaDll/libs/luasocket/select.c \
    LunaDll/libs/luasocket/serial.c \
    LunaDll/libs/luasocket/tcp.c \
    LunaDll/libs/luasocket/timeout.c \
    LunaDll/libs/luasocket/udp.c \
    LunaDll/libs/luasocket/unix.c \
    LunaDll/libs/luasocket/usocket.c \
    LunaDll/libs/luasocket/wsocket.c

