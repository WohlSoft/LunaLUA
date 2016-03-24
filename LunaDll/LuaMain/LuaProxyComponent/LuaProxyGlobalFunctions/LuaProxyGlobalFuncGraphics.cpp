#include "../../LuaProxy.h"
#include "../../../Globals.h"
#include "../../../GlobalFuncs.h"
#include "../../../Rendering/Rendering.h"
#include "../../../Misc/RuntimeHook.h"
#include "../../../Rendering/RenderOps/RenderBitmapOp.h"
#include "../../../Rendering/RenderOps/RenderStringOp.h"
#include "../../../SMBXInternal/CameraInfo.h"
#include "../../../Rendering/GL/GLEngineProxy.h"
#include "../../../Rendering/SMBXMaskedImage.h"
#include "../../../SMBXInternal/HardcodedGraphicsAccess.h"
#include <luabind/adopt_policy.hpp>
#include <luabind/out_value_policy.hpp>
// Stores reference to a loaded image
LuaProxy::Graphics::LuaImageResource::LuaImageResource(const std::shared_ptr<BMPBox>& img) {
    this->img = img;
}



// Deconstructor for when a loaded image resource is no longer referenced by Lua
LuaProxy::Graphics::LuaImageResource::~LuaImageResource() {
}

int LuaProxy::Graphics::LuaImageResource::GetWidth() const {
    if (!img) return 0;
    return img->m_W;
}

int LuaProxy::Graphics::LuaImageResource::GetHeight() const {
    if (!img) return 0;
    return img->m_H;
}


uintptr_t LuaProxy::Graphics::LuaImageResource::__BMPBoxPtr()
{
    return (uintptr_t)img.get();
}

void  LuaProxy::Graphics::LuaImageResource::setScaleUpMode(int m) {
	if (img)img->setScaleUpMode(m);
}

void  LuaProxy::Graphics::LuaImageResource::setScaleDownMode(int m) {
	if (img)img->setScaleDownMode(m);
}

void LuaProxy::Graphics::LuaMovieResource::play() {
	if (img)img->play();
}

void LuaProxy::Graphics::LuaMovieResource::stop() {
	if (img)img->stop();
}

void LuaProxy::Graphics::LuaMovieResource::pause() {
	if (img)img->pause();
}

void LuaProxy::Graphics::LuaMovieResource::seek(double sec) {
	if (img)img->seek(sec);
}
/*
void LuaProxy::Graphics::LuaMovieResource::setCallback(int idx,luabind::object cl,lua_State* L) {
	if (img && 0<=idx && idx <8 && luabind::type(cl) == LUA_TFUNCTION) {
		
		img->callbacks[idx] = gLunaLua.registerPassedLuaObj(idx,L,cl);
	}
}
*/
/*
luabind::object LuaProxy::Graphics::LuaMovieResource::getCallback(int idx, lua_State* L) {
	if (img && 0 <= idx && idx <8) {
		return img->callbacks[idx];
	}
}
*/
void LuaProxy::Graphics::LuaMovieResource::setMaskThreshold(int idx,int value) { if (img&& 0 <= idx && idx <8)img->maskThreshold[idx] = value; }
int LuaProxy::Graphics::LuaMovieResource::getMaskThreshold(int idx) const { if (img && 0 <= idx && idx <8)return img->maskThreshold[idx]; }
void LuaProxy::Graphics::LuaMovieResource::setVideoDelay(double d) { if (img)img->setVideoDelay(d); }
double LuaProxy::Graphics::LuaMovieResource::getVideoDelay() { return img ? img->getVideoDelay():0; }
void LuaProxy::Graphics::LuaMovieResource::setMaskDelay(double d) { if (img)img->setMaskDelay(d); }
double LuaProxy::Graphics::LuaMovieResource::getMaskDelay() { return img ? img->getMaskDelay() : 0; }
void LuaProxy::Graphics::LuaMovieResource::setScaleUpMode(int m) { if (img)img->setScaleUpMode(m); }
int LuaProxy::Graphics::LuaMovieResource::getScaleUpMode() { return img ? img->getScaleUpMode() : -1; }
void LuaProxy::Graphics::LuaMovieResource::setScaleDownMode(int m) { if (img)img->setScaleDownMode(m); }
int LuaProxy::Graphics::LuaMovieResource::getScaleDownMode() { return img ? img->getScaleDownMode() : -1; }
void LuaProxy::Graphics::LuaMovieResource::setOffScreenMode(int m) { if (img)img->setOffScreenMode(m); }
int LuaProxy::Graphics::LuaMovieResource::getOffScreenMode() { return img ? img->getOffScreenMode() : -1; }
void LuaProxy::Graphics::LuaMovieResource::setOnScreenMode(int m) { if (img)img->setOnScreenMode(m); }
int LuaProxy::Graphics::LuaMovieResource::getOnScreenMode() { return img ? img->getOnScreenMode() : -1; }
void LuaProxy::Graphics::LuaMovieResource::setLoop(bool enable) { if (img)img->setLoop(enable); }
bool LuaProxy::Graphics::LuaMovieResource::getLoop() { return img ? img->getLoop() : false; }
void LuaProxy::Graphics::LuaMovieResource::setAltAlpha(int ch) { if (img)img->setAltAlpha(ch); }
int LuaProxy::Graphics::LuaMovieResource::getAltAlpha() { return img ? img->getAltAlpha() : -1; }
void LuaProxy::Graphics::LuaMovieResource::setAlphaType(int m) { if (img)img->setAlphaType(m); }
int LuaProxy::Graphics::LuaMovieResource::getAlphaType() { return img ? img->getAlphaType() : 0; }
void LuaProxy::Graphics::LuaMovieResource::setVolume(int m) { if (img)img->setVolume(m); }
int LuaProxy::Graphics::LuaMovieResource::getVolume() { return img ? img->getVolume() : 0; }


void LuaProxy::Graphics::activateHud(bool activate)
{
    gSMBXHUDSettings.skip = !activate;
}

bool LuaProxy::Graphics::isHudActivated()
{
    return !gSMBXHUDSettings.skip;
}

void LuaProxy::Graphics::activateOverworldHud(WORLD_HUD_CONTROL activateFlag)
{
    gSMBXHUDSettings.overworldHudState = activateFlag;
}

WORLD_HUD_CONTROL LuaProxy::Graphics::getOverworldHudState()
{
    return gSMBXHUDSettings.overworldHudState;
}
std::shared_ptr<BMPBox> LuaProxy::Graphics::loadMedia(const std::string& filename) {
	std::wstring full_path;

	if (!isAbsolutePath(filename)) {
		full_path = getCustomFolderPath() + Str2WStr(filename);
	}
	else
	{
		full_path = Str2WStr(filename);
	}

	std::shared_ptr<BMPBox> img = std::make_shared<BMPBox>(full_path, gLunaRender.GetScreenDC());

	if (img->ImageLoaded() == false) {
		// If image loading failed, return null
		return NULL;
	}
	return img;
}
LuaProxy::Graphics::LuaImageResource* LuaProxy::Graphics::loadImage(const std::string& filename, lua_State* L)
{
	/*
    std::wstring full_path;

    if (!isAbsolutePath(filename)) {
        full_path = getCustomFolderPath() + Str2WStr(filename);
    }
    else
    {
        full_path = Str2WStr(filename);
    }

    std::shared_ptr<BMPBox> img = std::make_shared<BMPBox>(full_path, gLunaRender.GetScreenDC());

    if (img->ImageLoaded() == false) {
        // If image loading failed, return null
        return NULL;
    }
	*/
	std::shared_ptr<BMPBox> img = loadMedia(filename);
	if (img == NULL)return NULL;

    // Allocate a LuaImageResource to allow us to automatically garbage collect the image when no longer referenced in Lua
    return new LuaProxy::Graphics::LuaImageResource(img);
}

LuaProxy::Graphics::LuaMovieResource* LuaProxy::Graphics::loadMovie(const std::string& filename, lua_State* L)
{
	std::shared_ptr<BMPBox> img = loadMedia(filename);
	if (img == NULL)return NULL;

	// Allocate a LuaImageResource to allow us to automatically garbage collect the image when no longer referenced in Lua
	return new LuaProxy::Graphics::LuaMovieResource(img);
}

luabind::object LuaProxy::Graphics::loadAnimatedImage(const std::string& filename, int& smbxFrameTime, lua_State* L)
{
    luabind::object tLuaImageResources = luabind::newtable(L);
    std::vector<std::shared_ptr<BMPBox>> frames = gLunaRender.LoadAnimatedBitmapResource(Str2WStr(filename), &smbxFrameTime);
    for (unsigned int i = 0; i < frames.size(); i++){
        tLuaImageResources[i + 1] = luabind::object(L, new LuaProxy::Graphics::LuaImageResource(frames[i]), luabind::adopt(luabind::result));
    }
    return tLuaImageResources;
}



bool LuaProxy::Graphics::loadImage(const std::string& filename, int resNumber, int transColor)
{
    return gLunaRender.LoadBitmapResource(Str2WStr(filename), resNumber, transColor);
}


void LuaProxy::Graphics::placeSprite(int type, int imgResource, int xPos, int yPos, const std::string& extra, int time)
{
    CSpriteRequest req;
    req.type = type;
    req.img_resource_code = imgResource;
    req.direct_img = nullptr;
    req.x = xPos;
    req.y = yPos;
    req.time = time;
    req.str = Str2WStr(extra);
    gSpriteMan.InstantiateSprite(&req, false);
}

void LuaProxy::Graphics::placeSprite(int type, int imgResource, int xPos, int yPos, const std::string& extra)
{
    placeSprite(type, imgResource, xPos, yPos, extra, 0);
}


void LuaProxy::Graphics::placeSprite(int type, int imgResource, int xPos, int yPos)
{
    placeSprite(type, imgResource, xPos, yPos, "");
}

void LuaProxy::Graphics::placeSprite(int type, const LuaProxy::Graphics::LuaImageResource& img, int xPos, int yPos, const std::string& extra, int time)
{
    CSpriteRequest req;
    req.type = type;
    req.img_resource_code = -1;
    req.direct_img = img.img;
    req.x = xPos;
    req.y = yPos;
    req.time = time;
    req.str = Str2WStr(extra);
    gSpriteMan.InstantiateSprite(&req, false);
}

void LuaProxy::Graphics::placeSprite(int type, const LuaProxy::Graphics::LuaImageResource& img, int xPos, int yPos, const std::string& extra)
{
    placeSprite(type, img.img, xPos, yPos, extra, 0);
}

void LuaProxy::Graphics::placeSprite(int type, const LuaProxy::Graphics::LuaImageResource& img, int xPos, int yPos)
{
    placeSprite(type, img.img, xPos, yPos, "");
}


void LuaProxy::Graphics::unplaceSprites(const LuaImageResource& img, int xPos, int yPos)
{
    gSpriteMan.ClearSprites(img.img, xPos, yPos);
}

void LuaProxy::Graphics::unplaceSprites(const LuaImageResource& img)
{
    gSpriteMan.ClearSprites(img.img);
}

luabind::object LuaProxy::Graphics::getPixelData(const LuaImageResource& img, int& width, int& height, lua_State *L)
{
    if (!img.img || !img.img->ImageLoaded()){
        luaL_error(L, "Internal error: Failed to find image resource!");
        return luabind::object();
    }

    luabind::object returnTable = luabind::newtable(L);
    int i = 1;
    img.img->forEachPixelValue([&returnTable, &i](BYTE nextPixelValue){returnTable[i++] = nextPixelValue; });
    width = img.img->m_W;
    height = img.img->m_H;

    return returnTable;
}


void LuaProxy::Graphics::drawImage(const LuaImageResource& img, double xPos, double yPos, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, 0, 0, 0, 0, 1.0f, false, RENDEROP_DEFAULT_PRIORITY_RENDEROP, L);
}

void LuaProxy::Graphics::drawImage(const LuaImageResource& img, double xPos, double yPos, float opacity, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, 0, 0, 0, 0, opacity, false, RENDEROP_DEFAULT_PRIORITY_RENDEROP, L);
}

void LuaProxy::Graphics::drawImage(const LuaImageResource& img, double xPos, double yPos, double sourceX, double sourceY, double sourceWidth, double sourceHeight, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, sourceX, sourceY, sourceWidth, sourceHeight, 1.0f, false, RENDEROP_DEFAULT_PRIORITY_RENDEROP, L);
}

void LuaProxy::Graphics::drawImage(const LuaImageResource& img, double xPos, double yPos, double sourceX, double sourceY, double sourceWidth, double sourceHeight, float opacity, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, sourceX, sourceY, sourceWidth, sourceHeight, opacity, false, RENDEROP_DEFAULT_PRIORITY_RENDEROP, L);
}

void LuaProxy::Graphics::drawImageWP(const LuaImageResource& img, double xPos, double yPos, double priority, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, 0, 0, 0, 0, 1.0f, false, priority, L);
}

void LuaProxy::Graphics::drawImageWP(const LuaImageResource& img, double xPos, double yPos, float opacity, double priority, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, 0, 0, 0, 0, opacity, false, priority, L);
}

void LuaProxy::Graphics::drawImageWP(const LuaImageResource& img, double xPos, double yPos, double sourceX, double sourceY, double sourceWidth, double sourceHeight, double priority, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, sourceX, sourceY, sourceWidth, sourceHeight, 1.0f, false, priority, L);
}

void LuaProxy::Graphics::drawImageWP(const LuaImageResource& img, double xPos, double yPos, double sourceX, double sourceY, double sourceWidth, double sourceHeight, float opacity, double priority, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, sourceX, sourceY, sourceWidth, sourceHeight, opacity, false, priority, L);
}

void LuaProxy::Graphics::drawImageScaleWP(const LuaImageResource& img, double xPos, double yPos,double destWidth,double destHeight, double priority, lua_State* L)
{
	drawImageGeneric(img, xPos, yPos, 0, 0, 0, 0,destWidth,destHeight, 1.0f, false, priority, L);
}
void LuaProxy::Graphics::drawImageScaleWP(const LuaImageResource& img, double xPos, double yPos, double destWidth, double destHeight, float opacity, double priority, lua_State* L) {
	drawImageGeneric(img, xPos, yPos, 0, 0, 0, 0, destWidth, destHeight, opacity, false, priority, L);
}
void LuaProxy::Graphics::drawImageScaleWP(const LuaImageResource& img, double xPos, double yPos, double sourceX, double sourceY, double sourceWidth, double sourceHeight, double destWidth, double destHeight, double priority, lua_State* L) {
	drawImageGeneric(img, xPos, yPos, sourceX, sourceY, sourceWidth, sourceHeight, destWidth, destHeight, 1.0f, false, priority, L);
}
void LuaProxy::Graphics::drawImageScaleWP(const LuaImageResource& img, double xPos, double yPos, double sourceX, double sourceY, double sourceWidth, double sourceHeight, double destWidth, double destHeight, float opacity, double priority, lua_State* L) {
	drawImageGeneric(img, xPos, yPos, sourceX, sourceY, sourceWidth, sourceHeight, destWidth, destHeight, opacity, false, priority, L);
}



void LuaProxy::Graphics::drawImageToScene(const LuaImageResource& img, double xPos, double yPos, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, 0, 0, 0, 0, 1.0f, true, RENDEROP_DEFAULT_PRIORITY_RENDEROP, L);
}

void LuaProxy::Graphics::drawImageToScene(const LuaImageResource& img, double xPos, double yPos, float opacity, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, 0, 0, 0, 0, opacity, true, RENDEROP_DEFAULT_PRIORITY_RENDEROP, L);
}

void LuaProxy::Graphics::drawImageToScene(const LuaImageResource& img, double xPos, double yPos, double sourceX, double sourceY, double sourceWidth, double sourceHeight, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, sourceX, sourceY, sourceWidth, sourceHeight, 1.0f, true, RENDEROP_DEFAULT_PRIORITY_RENDEROP, L);
}

void LuaProxy::Graphics::drawImageToScene(const LuaImageResource& img, double xPos, double yPos, double sourceX, double sourceY, double sourceWidth, double sourceHeight, float opacity, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, sourceX, sourceY, sourceWidth, sourceHeight, opacity, true, RENDEROP_DEFAULT_PRIORITY_RENDEROP, L);
}

void LuaProxy::Graphics::drawImageToSceneWP(const LuaImageResource& img, double xPos, double yPos, double priority, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, 0, 0, 0, 0, 1.0f, true, priority, L);
}

void LuaProxy::Graphics::drawImageToSceneWP(const LuaImageResource& img, double xPos, double yPos, float opacity, double priority, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, 0, 0, 0, 0, opacity, true, priority, L);
}

void LuaProxy::Graphics::drawImageToSceneWP(const LuaImageResource& img, double xPos, double yPos, double sourceX, double sourceY, double sourceWidth, double sourceHeight, double priority, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, sourceX, sourceY, sourceWidth, sourceHeight, 1.0f, true, priority, L);
}

void LuaProxy::Graphics::drawImageToSceneWP(const LuaImageResource& img, double xPos, double yPos, double sourceX, double sourceY, double sourceWidth, double sourceHeight, float opacity, double priority, lua_State* L)
{
    drawImageGeneric(img, xPos, yPos, sourceX, sourceY, sourceWidth, sourceHeight, opacity, true, priority, L);
}

void LuaProxy::Graphics::drawImageGeneric(const LuaImageResource& img, double xPos, double yPos, double sourceX, double sourceY, double sourceWidth, double sourceHeight, float opacity, bool sceneCoords, double priority, lua_State* L)
{
    if (priority < RENDEROP_PRIORITY_MIN || priority > RENDEROP_PRIORITY_MAX) {
        luaL_error(L, "Priority value is not valid (must be between %f and %f, got %f).", RENDEROP_PRIORITY_MIN, RENDEROP_PRIORITY_MAX, priority);
        return;
    }

    RenderBitmapOp* renderOp = new RenderBitmapOp();
    renderOp->direct_img = img.img;
    renderOp->x = xPos;
    renderOp->y = yPos;
    renderOp->sx = (sourceX <= 0.0 ? 0.0 : sourceX);
    renderOp->sy = (sourceY <= 0.0 ? 0.0 : sourceY);
    renderOp->sw = (sourceWidth <= 0.0 ? static_cast<double>(img.img->m_W) : sourceWidth);
    renderOp->sh = (sourceHeight <= 0.0 ? static_cast<double>(img.img->m_H) : sourceHeight);
    renderOp->opacity = opacity;
    renderOp->sceneCoords = sceneCoords;
    renderOp->m_renderPriority = priority; 

    gLunaRender.AddOp(renderOp);
}

void LuaProxy::Graphics::drawImageGeneric(const LuaImageResource& img, double xPos, double yPos, double sourceX, double sourceY, double sourceWidth, double sourceHeight,double destWidth,double destHeight, float opacity, bool sceneCoords, double priority, lua_State* L)
{
	if (priority < RENDEROP_PRIORITY_MIN || priority > RENDEROP_PRIORITY_MAX) {
		luaL_error(L, "Priority value is not valid (must be between %f and %f, got %f).", RENDEROP_PRIORITY_MIN, RENDEROP_PRIORITY_MAX, priority);
		return;
	}

	RenderBitmapOp* renderOp = new RenderBitmapOp();
	renderOp->direct_img = img.img;
	renderOp->x = xPos;
	renderOp->y = yPos;
	renderOp->sx = (sourceX <= 0.0 ? 0.0 : sourceX);
	renderOp->sy = (sourceY <= 0.0 ? 0.0 : sourceY);
	renderOp->sw = (sourceWidth <= 0.0 ? static_cast<double>(img.img->m_W) : sourceWidth);
	renderOp->sh = (sourceHeight <= 0.0 ? static_cast<double>(img.img->m_H) : sourceHeight);
	renderOp->dw = (destWidth <= 0.0 ? renderOp->sw : destWidth);
	renderOp->dh = (destHeight <= 0.0 ? renderOp->sh : destHeight);
	renderOp->scaling = true;
	renderOp->opacity = opacity;
	renderOp->sceneCoords = sceneCoords;
	renderOp->m_renderPriority = priority;

	gLunaRender.AddOp(renderOp);
}


void LuaProxy::Graphics::draw(const luabind::object& namedArgs, lua_State* L)
{
    if (luabind::type(namedArgs) != LUA_TTABLE) {
        luaL_error(L, "Argument #1 must be a table with named arguments!");
        return;
    }
    

    double x, y;
    RENDER_TYPE type;
    double priority;
    RenderOp* renderOperation;
    bool isSceneCoordinates;
    LUAHELPER_GET_NAMED_ARG_OR_RETURN_VOID(namedArgs, x);
    LUAHELPER_GET_NAMED_ARG_OR_RETURN_VOID(namedArgs, y);
    LUAHELPER_GET_NAMED_ARG_OR_RETURN_VOID(namedArgs, type);
    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, isSceneCoordinates, false);
    if (type == RTYPE_TEXT) 
    {
        priority = RENDEROP_DEFAULT_PRIORITY_TEXT;

        std::string text;
        int fontType;
        LUAHELPER_GET_NAMED_ARG_OR_RETURN_VOID(namedArgs, text);
        LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, fontType, 3);

        RenderStringOp* strRenderOp = new RenderStringOp();
        strRenderOp->m_String = Str2WStr(text);
        if (fontType == 3)
            for (auto& nextChar : strRenderOp->m_String)
                nextChar = towupper(nextChar);
        strRenderOp->m_X = static_cast<float>(x);
        strRenderOp->m_Y = static_cast<float>(y);
        strRenderOp->m_FontType = fontType;
        strRenderOp->sceneCoords = isSceneCoordinates;
        renderOperation = strRenderOp;
    }
    else if (type == RTYPE_IMAGE) 
    {
        priority = RENDEROP_DEFAULT_PRIORITY_RENDEROP;
        
        LuaImageResource* image;
        double sourceX;
        double sourceY;
        double sourceWidth;
        double sourceHeight;
        float opacity;
        
        LUAHELPER_GET_NAMED_ARG_OR_RETURN_VOID(namedArgs, image);

        if (!image || !image->img || !image->img->ImageLoaded()) {
            luaL_error(L, "Image may not be nil.");
            return;
        }

        LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, sourceX, 0.0);
        LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, sourceY, 0.0);
        LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, sourceWidth, static_cast<double>(image->img->m_W));
        LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, sourceHeight, static_cast<double>(image->img->m_H));
        LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, opacity, 1.0f);
        
        RenderBitmapOp* bitmapRenderOp = new RenderBitmapOp();
        bitmapRenderOp->direct_img = image->img;
        bitmapRenderOp->sx = sourceX;
        bitmapRenderOp->sy = sourceY;
        bitmapRenderOp->sw = sourceWidth;
        bitmapRenderOp->sh = sourceHeight;
        bitmapRenderOp->x = x;
        bitmapRenderOp->y = y;
        bitmapRenderOp->sceneCoords = isSceneCoordinates;
        bitmapRenderOp->opacity = opacity;
        renderOperation = bitmapRenderOp;
    }
    else
    {
        luaL_error(L, "No valid 'type'. Must be RTYPE_TEXT or RTYPE_IMAGE");
        return;
    }
    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, priority, priority);
 
    renderOperation->m_renderPriority = priority;
    gLunaRender.AddOp(renderOperation);
}



bool LuaProxy::Graphics::isOpenGLEnabled()
{
    return g_GLEngine.IsEnabled();
}

void LuaProxy::Graphics::glSetTexture(const LuaImageResource* img, uint32_t color)
{   
    // Convert RGB to RGBA
    LuaProxy::Graphics::glSetTextureRGBA(img, (color << 8) | 0xFF);
}

void LuaProxy::Graphics::glSetTextureRGBA(const LuaImageResource* img, uint32_t color)
{
    const BMPBox* bmp = NULL;
    if (img && img->img && img->img->ImageLoaded()) {
        bmp = img->img.get(); // Get a raw pointer, because currently the BMPBox destructor tells the render thread to cut it out
    }

    auto obj = std::make_shared<GLEngineCmd_SetTexture>();
    obj->mBmp = bmp;
    obj->mColor = color;
    gLunaRender.GLCmd(obj);
}

void LuaProxy::Graphics::__glInternalDraw(const luabind::object& namedArgs, lua_State* L)
{
    double priority;
    const LuaProxy::Graphics::LuaImageResource* luaImageResource;
    std::shared_ptr<CaptureBuffer> capBuff = nullptr;
    float r, g, b, a;
    unsigned int rawVer, rawTex, rawCol, rawCnt;
    unsigned int primitive;
    bool sceneCoords;

    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, priority, RENDEROP_DEFAULT_PRIORITY_RENDEROP);
    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, primitive, GL_TRIANGLES);
    {
        const LuaProxy::Graphics::LuaImageResource* texture;
        LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_NOERROR(namedArgs, texture, nullptr);
        luaImageResource = texture;
    }
    if (luaImageResource == nullptr) {
        std::shared_ptr<CaptureBuffer> texture;
        LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, texture, nullptr);
        capBuff = texture;
    }
    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, rawVer, (unsigned int)nullptr);
    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, rawTex, (unsigned int)nullptr);
    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, rawCol, (unsigned int)nullptr);
    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, rawCnt, 0);
    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, r, 1.0);
    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, g, 1.0);
    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, b, 1.0);
    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, a, 1.0);
    LUAHELPER_GET_NAMED_ARG_OR_DEFAULT_OR_RETURN_VOID(namedArgs, sceneCoords, false);

    const BMPBox* bmp = nullptr;
    if (luaImageResource && luaImageResource->img && luaImageResource->img->ImageLoaded()) {
        bmp = luaImageResource->img.get(); // Get a raw pointer, because currently the BMPBox destructor tells the render thread to cut it out
    }

    auto obj = std::make_shared<GLEngineCmd_LuaDraw>();
    obj->mBmp = bmp;
    obj->mCapBuff = capBuff;
    obj->mColor[0] = r;
    obj->mColor[1] = g;
    obj->mColor[2] = b;
    obj->mColor[3] = a;
    obj->mType = primitive;
    obj->mVert = (const float*)rawVer;
    obj->mTex = (const float*)rawTex;
    obj->mVertColor = (const float*)rawCol;
    obj->mCount = rawCnt;
    obj->mSceneCoords = sceneCoords;
    gLunaRender.GLCmd(obj, priority);
}

static SMBXMaskedImage* getMaskedImage(const std::string& t, int index)
{
    HDC* mainArray = nullptr;
    HDC* maskArray = nullptr;
    HDC mainHdc;
    HDC maskHdc;
    int maxIndex = 0;

    if (t == "block")
    {
        mainArray = GM_GFX_BLOCKS_PTR;
        maskArray = GM_GFX_BLOCKS_MASK_PTR;
        maxIndex = 700;
    }
    else if (t == "background2")
    {
        mainArray = GM_GFX_BACKGROUND2_PTR;
        maxIndex = 58;
    }
    else if (t == "npc")
    {
        mainArray = GM_GFX_NPC_PTR;
        maskArray = GM_GFX_NPC_MASK_PTR;
        maxIndex = 300;
    }
    else if (t == "effect")
    {
        mainArray = GM_GFX_EFFECTS_PTR;
        maskArray = GM_GFX_EFFECTS_MASK_PTR;
        maxIndex = 148;
    }
    else if (t == "background")
    {
        mainArray = GM_GFX_BACKGROUND_PTR;
        maskArray = GM_GFX_BACKGROUND_MASK_PTR;
        maxIndex = 190;
    }
    else if (t == "mario")
    {
        mainArray = GM_GFX_MARIO_PTR;
        maskArray = GM_GFX_MARIO_MASK_PTR;
        maxIndex = 7;
    }
    else if (t == "luigi")
    {
        mainArray = GM_GFX_LUIGI_PTR;
        maskArray = GM_GFX_LUIGI_MASK_PTR;
        maxIndex = 7;
    }
    else if (t == "peach")
    {
        mainArray = GM_GFX_PEACH_PTR;
        maskArray = GM_GFX_PEACH_MASK_PTR;
        maxIndex = 7;
    }
    else if (t == "toad")
    {
        mainArray = GM_GFX_TOAD_PTR;
        maskArray = GM_GFX_TOAD_MASK_PTR;
        maxIndex = 7;
    }
    else if (t == "link")
    {
        mainArray = GM_GFX_LINK_PTR;
        maskArray = GM_GFX_LINK_MASK_PTR;
        maxIndex = 7;
    }
    else if (t == "yoshib")
    {
        mainArray = GM_GFX_YOSHIB_PTR;
        maskArray = GM_GFX_YOSHIB_MASK_PTR;
        maxIndex = 8;
    }
    else if (t == "yoshit")
    {
        mainArray = GM_GFX_YOSHIT_PTR;
        maskArray = GM_GFX_YOSHIT_MASK_PTR;
        maxIndex = 8;
    }
    else if (t == "tile")
    {
        mainArray = GM_GFX_TILES_PTR;
        maskArray = nullptr;
        maxIndex = 328;
    }
    else if (t == "level")
    {
        mainArray = GM_GFX_LEVEL_PTR;
        maskArray = GM_GFX_LEVEL_MASK_PTR;
        maxIndex = 32;
    }
    else if (t == "scene")
    {
        mainArray = GM_GFX_SCENE_PTR;
        maskArray = GM_GFX_SCENE_MASK_PTR;
        maxIndex = 65;
    }
    else if (t == "path")
    {
        mainArray = GM_GFX_PATH_PTR;
        maskArray = GM_GFX_PATH_MASK_PTR;
        maxIndex = 32;
    }
    else if (t == "player")
    {
        mainArray = GM_GFX_PLAYER_PTR;
        maskArray = GM_GFX_PLAYER_MASK_PTR;
        maxIndex = 5;
    }

    // Check range on index and get HDCs
    if (index < 1 || index > maxIndex) return nullptr;
    mainHdc = (mainArray != nullptr) ? mainArray[index - 1] : nullptr;
    maskHdc = (maskArray != nullptr) ? maskArray[index - 1] : nullptr;

    // If we have no HDC abort
    if (mainHdc == nullptr && maskHdc == nullptr) return nullptr;

    // Get the image
    return SMBXMaskedImage::get(maskHdc, mainHdc);
}

void LuaProxy::Graphics::__setSpriteOverride(const std::string& t, int index, const luabind::object& overrideImg, lua_State* L)
{
    SMBXMaskedImage* img = getMaskedImage(t, index);
    if (img == nullptr)
    {
        luaL_error(L, "Graphics.sprite.%s[%d] does not exist", t.c_str(), index);
        return;
    }

    if (!overrideImg.is_valid())
    {
        img->UnsetOverride();
        return;
    }

	//I found that 'object_cast_nothrow' sometimes casts between incompatible types without returning 'boost::none'
	//switch order to avoid invalid casting from LIR to SMBXMI
	boost::optional<LuaProxy::Graphics::LuaImageResource*> rgbaImg = luabind::object_cast_nothrow<LuaProxy::Graphics::LuaImageResource*>(overrideImg);
	if (rgbaImg != boost::none) {

		if (*rgbaImg != nullptr && (*rgbaImg)->img) {
			img->SetOverride((*rgbaImg)->img);
		}
		return;
	}


	boost::optional<SMBXMaskedImage*> maskImg = luabind::object_cast_nothrow<SMBXMaskedImage*>(overrideImg);
	if (maskImg != boost::none) {
		img->SetOverride(*maskImg);
		return;
	}

	

    

    

    luaL_error(L, "Cannot set Graphics.sprite.%s[%d], invalid input type", t.c_str(), index);
}

void LuaProxy::Graphics::__setSimpleSpriteOverride(const std::string & name, const luabind::object & overrideImg, lua_State * L)
{
    HDC mainHdc, maskHdc;
    if (name.find("hardcoded-") == 0)
    {
        // If non of the above applies, then try with the hardcoded ones:
        HardcodedGraphicsItem::GetHDCByName(name, &mainHdc, &maskHdc);
    }
    if (mainHdc == nullptr && maskHdc == nullptr) {
        luaL_error(L, "Failed to get hardcoded image!");
        return;
    }
    
    SMBXMaskedImage* img = SMBXMaskedImage::get(mainHdc, maskHdc);
    if (!overrideImg.is_valid())
    {
        img->UnsetOverride();
        return;
    }

    boost::optional<SMBXMaskedImage*> maskImg = luabind::object_cast_nothrow<SMBXMaskedImage*>(overrideImg);
    if (maskImg != boost::none) {
        img->SetOverride(*maskImg);
        return;
    }

    boost::optional<LuaProxy::Graphics::LuaImageResource*> rgbaImg = luabind::object_cast_nothrow<LuaProxy::Graphics::LuaImageResource*>(overrideImg);
    if (rgbaImg != boost::none) {
        if (*rgbaImg != nullptr && (*rgbaImg)->img) {
            img->SetOverride((*rgbaImg)->img);
        }
        return;
    }
    luaL_error(L, "Invalid input for sprite override!");
}

luabind::object LuaProxy::Graphics::__getSpriteOverride(const std::string& t, int index, lua_State* L)
{
    SMBXMaskedImage* img = getMaskedImage(t, index);
    if (img == nullptr)
    {
        luaL_error(L, "Graphics.sprite.%s[%d] does not exist", t.c_str(), index);
        return luabind::object();
    }

    SMBXMaskedImage* maskOverride = img->GetMaskedOverride();
    if (maskOverride != nullptr)
    {
        return luabind::object(L, maskOverride);
    }

    std::shared_ptr<BMPBox> rgbaOverride = img->GetRGBAOverride();
    if (rgbaOverride)
    {
        return luabind::object(L, new LuaImageResource(rgbaOverride), luabind::adopt(luabind::result));
    }

    std::shared_ptr<BMPBox> loadedPng = img->GetLoadedPng();
	/*
	FIXED?

	if(rgbaOverride) -> if(loadedPng)

	*/
	if (loadedPng)
    {
        return luabind::object(L, new LuaImageResource(loadedPng), luabind::adopt(luabind::result));
    }

    return luabind::object(L, img);
}
