#include "../../LuaProxy.h"
#include "../../../Globals.h"
#include "../../../GlobalFuncs.h"
#include "../../../Rendering/Rendering.h"
#include "../../../Misc/RuntimeHook.h"
#include "../../../Rendering/RenderOps/RenderBitmapOp.h"
#include "../../../SMBXInternal/CameraInfo.h"

// Stores reference to a loaded image
LuaProxy::Graphics::LuaImageResource::LuaImageResource(int imgResource) {
    this->imgResource = imgResource;
}

// Deconstructor for when a loaded image resource is no longer referenced by Lua
LuaProxy::Graphics::LuaImageResource::~LuaImageResource() {
    gLunaRender.DeleteImage(imgResource);
}

void LuaProxy::Graphics::activateHud(bool activate)
{
    gSkipSMBXHUD = !activate;
}

void LuaProxy::Graphics::activateOverworldHud(WORLD_HUD_CONTROL activateFlag)
{
    gOverworldHudControlFlag = activateFlag;
}

LuaProxy::Graphics::LuaImageResource* LuaProxy::Graphics::loadImage(const char* filename)
{
    int resNumber = gLunaRender.GetAutoImageResourceCode();
    if (resNumber == 0) return NULL;

    if (!gLunaRender.LoadBitmapResource(utf8_decode(std::string(filename)), resNumber)) {
        // If image loading failed, return null
        return NULL;
    }

    // Allocate a LuaImageResource to allow us to automatically garbage collect the image when no longer referenced in Lua
    return new LuaProxy::Graphics::LuaImageResource(resNumber);
}

bool LuaProxy::Graphics::loadImage(const char* filename, int resNumber, int transColor)
{
    return gLunaRender.LoadBitmapResource(utf8_decode(std::string(filename)), resNumber, transColor);
}


void LuaProxy::Graphics::placeSprite(int type, int imgResource, int xPos, int yPos, const char *extra, int time)
{
    CSpriteRequest req;
    req.type = type;
    req.img_resource_code = imgResource;
    req.x = xPos;
    req.y = yPos;
    req.time = time;
    req.str = utf8_decode(std::string(extra));
    gSpriteMan.InstantiateSprite(&req, false);
}

void LuaProxy::Graphics::placeSprite(int type, int imgResource, int xPos, int yPos, const char *extra)
{
    placeSprite(type, imgResource, xPos, yPos, extra, 0);
}


void LuaProxy::Graphics::placeSprite(int type, int imgResource, int xPos, int yPos)
{
    placeSprite(type, imgResource, xPos, yPos, "");
}

void LuaProxy::Graphics::placeSprite(int type, const LuaProxy::Graphics::LuaImageResource& img, int xPos, int yPos, const char *extra, int time)
{
    placeSprite(type, img.imgResource, xPos, yPos, extra, time);
}

void LuaProxy::Graphics::placeSprite(int type, const LuaProxy::Graphics::LuaImageResource& img, int xPos, int yPos, const char *extra)
{
    placeSprite(type, img.imgResource, xPos, yPos, extra, 0);
}

void LuaProxy::Graphics::placeSprite(int type, const LuaProxy::Graphics::LuaImageResource& img, int xPos, int yPos)
{
    placeSprite(type, img.imgResource, xPos, yPos, "");
}


void LuaProxy::Graphics::unplaceSprites(const LuaImageResource& img, int xPos, int yPos)
{
    gSpriteMan.ClearSprites(img.imgResource, xPos, yPos);
}

void LuaProxy::Graphics::unplaceSprites(const LuaImageResource& img)
{
    gSpriteMan.ClearSprites(img.imgResource);
}


void LuaProxy::Graphics::drawImage(const LuaImageResource& img, int xPos, int yPos, lua_State* L)
{
    drawImage(img, xPos, yPos, 0, 0, 0, 0, L);
}

void LuaProxy::Graphics::drawImage(const LuaImageResource& img, int xPos, int yPos, int sourceX, int sourceY, int sourceWidth, int sourceHeight, lua_State* L)
{
    const auto bmpIt = gLunaRender.LoadedImages.find(img.imgResource);
    if (bmpIt == gLunaRender.LoadedImages.cend()){
        luaL_error(L, "Internal error: Failed to find image resource!");
        return;
    }

    BMPBox* imgBox = bmpIt->second;
    
    RenderBitmapOp* renderOp = new RenderBitmapOp();
    renderOp->img_resource_code = img.imgResource;
    renderOp->x = xPos;
    renderOp->y = yPos;
    renderOp->sx1 = (sourceX <= 0 ? 0 : sourceX);
    renderOp->sy1 = (sourceY <= 0 ? 0 : sourceY);
    renderOp->sx2 = (sourceWidth <= 0 ? imgBox->m_W : sourceX + sourceWidth);
    renderOp->sy2 = (sourceHeight <= 0 ? imgBox->m_H : sourceY + sourceHeight);

    gLunaRender.AddOp(renderOp);
}

void LuaProxy::Graphics::drawImageToScene(const LuaImageResource& img, int xPos, int yPos, lua_State* L)
{
    drawImageToScene(img, xPos, yPos, 0, 0, 0, 0, L);
}

void LuaProxy::Graphics::drawImageToScene(const LuaImageResource& img, int xPos, int yPos, int sourceX, int sourceY, int sourceWidth, int sourceHeight, lua_State* L)
{
    double camX = SMBX_CameraInfo::getCameraX(1);
    double camY = SMBX_CameraInfo::getCameraY(1);
    
    drawImage(img, xPos - camX, yPos - camY, sourceX, sourceY, sourceWidth, sourceHeight, L);
}



void LuaProxy::Graphics::glSetTexture(const LuaImageResource* img, uint32_t color)
{   
    // Convert RGB to RGBA
    LuaProxy::Graphics::glSetTextureRGBA(img, (color << 8) | 0xFF);
}

void LuaProxy::Graphics::glSetTextureRGBA(const LuaImageResource* img, uint32_t color)
{
    const BMPBox* bmp = NULL;
    if (img) {
        auto it = gLunaRender.LoadedImages.find(img->imgResource);
        if (it != gLunaRender.LoadedImages.end()) {
            bmp = it->second;
        }
    }

    gLunaRender.GLCmd(GLEngineCmd::SetTex(bmp, color));
}

extern "C" {
    __declspec(dllexport) float* __cdecl LunaLuaGlAllocCoords(size_t size) {
        return (float*)malloc(size * sizeof(float));
    }

    __declspec(dllexport) void __cdecl LunaLuaGlDrawTriangles(const float* vert, const float* tex, unsigned int count) {
        gLunaRender.GLCmd(GLEngineCmd::DrawTriangles(vert, tex, count));
    }
}