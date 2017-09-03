#include <cstring>
#include "../Globals.h"
#include "../GlobalFuncs.h"
#include "ImageLoader.h"
#include "SMBXImageCategories.h"
#include "RenderUtils.h"
#include "../Misc/ResourceFileMapper.h"
#include "LunaImage.h"
#include "../SMBXInternal/HardcodedGraphicsAccess.h"
#include <Windows.h>

// Loaded image map decleration
std::unordered_map<std::string, std::shared_ptr<LunaImage>> ImageLoader::m_ExtraGfx;
std::unordered_map<std::string, std::shared_ptr<LunaImage>> ImageLoader::m_ExtraGfxOverride;
std::unordered_map<std::string, uintptr_t>                  ImageLoader::m_NameToHDC;
std::unordered_map<uintptr_t, std::shared_ptr<LunaImage>>   ImageLoader::m_GfxOverride;
std::unordered_map<uintptr_t, std::shared_ptr<LunaImage>>   ImageLoader::m_Gfx;

static bool checkDirectoryExistance(const std::wstring& path)
{
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fileData) == 0)
    {
        // Failed to get attributes
        return false;
    }

    if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        return true;
    }

    return false;
}

static void resolveImageResource(
    const std::wstring& appGfxTypeDir,
    const std::wstring& episodeGfxTypeDir,
    const std::wstring& fileRoot,
    const std::unordered_map<std::wstring, ResourceFileInfo>& levelFiles,
    const std::unordered_map<std::wstring, ResourceFileInfo>& episodeFiles,
    std::unordered_map<std::wstring, ResourceFileInfo>& outData)
{
    std::wstring pngName = fileRoot + L".png";
    std::wstring gifName = fileRoot + L".gif";

    bool mainIsGif;
    {
        ResourceFileInfo resource;
        auto it = levelFiles.find(pngName);
        if (it == levelFiles.end())
            it = levelFiles.find(gifName);
        if (it == levelFiles.end())
            it = episodeFiles.find(pngName);
        if (it == episodeFiles.end())
            it = episodeFiles.find(gifName);
        if (it != episodeFiles.end())
            resource = it->second;
        if (episodeGfxTypeDir.length() > 0)
        {
            if (resource.path.length() == 0)
                resource = GetResourceFileInfo(episodeGfxTypeDir, fileRoot, L"png");
            if (resource.path.length() == 0)
                resource = GetResourceFileInfo(episodeGfxTypeDir, fileRoot, L"gif");
        }
        if (appGfxTypeDir.length() > 0)
        {
            if (resource.path.length() == 0)
                resource = GetResourceFileInfo(appGfxTypeDir, fileRoot, L"png");
            if (resource.path.length() == 0)
                resource = GetResourceFileInfo(appGfxTypeDir, fileRoot, L"gif");
        }
        mainIsGif = (resource.extension == L"gif");
        if (resource.path.length() > 0)
        {
            outData[fileRoot] = std::move(resource);
        }
    }

    if (mainIsGif)
    {
        std::wstring maskName = fileRoot + L"m.gif";
        ResourceFileInfo maskResource;
        auto it = levelFiles.find(maskName);
        if (it == levelFiles.end())
            it = episodeFiles.find(maskName);
        if (it != episodeFiles.end())
            maskResource = it->second;
        if (episodeGfxTypeDir.length() > 0)
        {
            if (maskResource.path.length() == 0)
                maskResource = GetResourceFileInfo(episodeGfxTypeDir, fileRoot + L"m", L"gif");
        }
        if (appGfxTypeDir.length() > 0)
        {
            if (maskResource.path.length() == 0)
                maskResource = GetResourceFileInfo(appGfxTypeDir, fileRoot + L"m", L"gif");
        }
        if (maskResource.path.length() > 0) {
            outData[fileRoot + L"m"] = std::move(maskResource);
        }
    }
}

void ImageLoaderCategory::resolveResources(
    const std::wstring& appGfxDir,
    const std::wstring& episodeGfxDir,
    const std::unordered_map<std::wstring, ResourceFileInfo>& levelFiles,
    const std::unordered_map<std::wstring, ResourceFileInfo>& episodeFiles,
    std::unordered_map<std::wstring, ResourceFileInfo>& outData) const
{
    uint32_t firstIdx = m_Category.getFirstIdx();
    uint32_t lastIdx = m_Category.getLastIdx();
    std::wstring prefix = m_Category.getPrefix();

    std::wstring gfxTypeSubdir = std::wstring(m_Category.getFolderPrefix());
    std::wstring appGfxTypeDir = (appGfxDir.length() > 0) ? appGfxDir + L"/" + gfxTypeSubdir : L"";
    std::wstring episodeGfxTypeDir = (episodeGfxDir.length() > 0) ? episodeGfxDir + L"/" + gfxTypeSubdir : L"";

    for (uint32_t idx = firstIdx; idx <= lastIdx; idx++)
    {
        std::wstring fileRoot = prefix + L"-" + std::to_wstring(idx);
        resolveImageResource(appGfxTypeDir, episodeGfxTypeDir, fileRoot, levelFiles, episodeFiles, outData);
    }
}

static void getImageResource(const std::wstring& imageName,
    const std::unordered_map<std::wstring, ResourceFileInfo>& fileData,
    ResourceFileInfo& mainResource,
    ResourceFileInfo& maskResource)
{
    auto it = fileData.find(imageName);
    if (it == fileData.end()) return;
    mainResource = it->second;

    it = fileData.find(imageName + L"m");
    if (it == fileData.end()) return;
    maskResource = it->second;
}

void ImageLoaderCategory::updateLoadedImages(const std::unordered_map<std::wstring, ResourceFileInfo>* fileData, const std::unordered_map<std::wstring, ResourceFileInfo>* oldFileData)
{
    uint32_t firstIdx = m_Category.getFirstIdx();
    uint32_t lastIdx = m_Category.getLastIdx();

    std::wstring prefix = m_Category.getPrefix();
    for (uint32_t i = firstIdx; i <= lastIdx; i++)
    {
        std::wstring imageName = prefix + L"-" + std::to_wstring(i);
        ResourceFileInfo newMain, newMask;
        ResourceFileInfo oldMain, oldMask;
        getImageResource(imageName, *fileData, newMain, newMask);
        if (oldFileData != nullptr)
        {
            getImageResource(imageName, *oldFileData, oldMain, oldMask);
        }

        // If anything has changed, reload
        if ((oldFileData == nullptr) || (newMain != oldMain) || (newMask != oldMask))
        {
            uint32_t width = 0, height = 0;

            HDC mainImgHdc = nullptr;
            std::shared_ptr<LunaImage> mainImg = nullptr;
            if (m_Category.haveImagePtrArray())
            {
                // Make HDC if-needed
                mainImgHdc = m_Category.getImagePtr(i);
                if (mainImgHdc == nullptr)
                {
                    mainImgHdc = CreateCompatibleDC(NULL);
                    m_Category.setImagePtr(i, mainImgHdc);
                    ImageLoader::m_NameToHDC[WStr2Str(imageName)] = (uintptr_t)mainImgHdc;
                }

                // Try to load image
                if (newMain.path.length() > 0)
                {
                    mainImg = LunaImage::fromFile(newMain.path.c_str());
                }

                // Assign image, note size
                ImageLoader::m_Gfx[(uintptr_t)mainImgHdc] = mainImg;
                if (mainImg)
                {
                    if (width < mainImg->getW()) width = mainImg->getW();
                    if (height < mainImg->getH()) height = mainImg->getH();
                }
            }
            else
            {
                // That's required...
                throw;
            }

            if (m_Category.haveMaskPtrArray() && mainImg)
            {
                // Match HDC to non-mask version
                m_Category.setMaskPtr(i, mainImgHdc);

                // Try to load image
                std::shared_ptr<LunaImage> maskImg = nullptr;
                if (newMask.path.length() > 0)
                {
                    maskImg = LunaImage::fromFile(newMask.path.c_str());
                }

                // Assign image, note size
                mainImg->setMask(maskImg);
                if (maskImg)
                {
                    if (width < maskImg->getW()) width = maskImg->getW();
                    if (height < maskImg->getH()) height = maskImg->getH();
                }

                mainImg->tryMaskToRGBA();
            }

            m_Category.setWidth(i, (int16_t)min(width, 0x7FFF));
            m_Category.setHeight(i, (int16_t)min(height, 0x7FFF));
        }
    }
}


static ImageLoaderCategory smbxImageLoaderBlock(smbxImageCategoryBlock);
static ImageLoaderCategory smbxImageLoaderBackground2(smbxImageCategoryBackground2);
static ImageLoaderCategory smbxImageLoaderNpc(smbxImageCategoryNpc);
static ImageLoaderCategory smbxImageLoaderEffect(smbxImageCategoryEffect);
static ImageLoaderCategory smbxImageLoaderBackground(smbxImageCategoryBackground);
static ImageLoaderCategory smbxImageLoaderMario(smbxImageCategoryMario);
static ImageLoaderCategory smbxImageLoaderLuigi(smbxImageCategoryLuigi);
static ImageLoaderCategory smbxImageLoaderPeach(smbxImageCategoryPeach);
static ImageLoaderCategory smbxImageLoaderToad(smbxImageCategoryToad);
static ImageLoaderCategory smbxImageLoaderLink(smbxImageCategoryLink);
static ImageLoaderCategory smbxImageLoaderYoshiB(smbxImageCategoryYoshiB);
static ImageLoaderCategory smbxImageLoaderYoshiT(smbxImageCategoryYoshiT);
static ImageLoaderCategory smbxImageLoaderTile(smbxImageCategoryTile);
static ImageLoaderCategory smbxImageLoaderLevel(smbxImageCategoryLevel);
static ImageLoaderCategory smbxImageLoaderScene(smbxImageCategoryScene);
static ImageLoaderCategory smbxImageLoaderPath(smbxImageCategoryPath);
static ImageLoaderCategory smbxImageLoaderPlayer(smbxImageCategoryPlayer);
static ImageLoaderCategory* smbxImageLoaderCategories[] = {
    &smbxImageLoaderBlock,
    &smbxImageLoaderBackground2,
    &smbxImageLoaderNpc,
    &smbxImageLoaderEffect,
    &smbxImageLoaderBackground,
    &smbxImageLoaderMario,
    &smbxImageLoaderLuigi,
    &smbxImageLoaderPeach,
    &smbxImageLoaderToad,
    &smbxImageLoaderLink,
    &smbxImageLoaderYoshiB,
    &smbxImageLoaderYoshiT,
    &smbxImageLoaderTile,
    &smbxImageLoaderLevel,
    &smbxImageLoaderScene,
    &smbxImageLoaderPath,
    &smbxImageLoaderPlayer,
    nullptr
};

void ImageLoader::Run(bool initialLoad)
{

    static std::unordered_map<std::wstring, ResourceFileInfo>* lastResources = nullptr;
    std::unordered_map<std::wstring, ResourceFileInfo>* foundResources = nullptr;
    foundResources = new std::unordered_map<std::wstring, ResourceFileInfo>();

    // Time to start figuring out our paths...

    std::wstring episodePath = normalizePathSlashes(GM_FULLDIR);
    std::wstring appPath = normalizePathSlashes(gAppPathWCHAR);
    std::wstring episodeGfxDir = L"";
    std::wstring appGfxDir = appPath + L"/graphics";
    std::wstring levelGfxDir = L"";

    // If we're not actually in an episode, make don't have an episode path
    if (appPath == episodePath)
    {
        episodePath = L"";
    }

    // Check if we have a graphics subdirectory in the episode
    if (episodePath.length() > 0)
    {
        std::wstring testSubdir = episodePath + L"/graphics";
        if (checkDirectoryExistance(testSubdir))
        {
            episodeGfxDir = testSubdir;
        }
    }

    // If not in the overworld, we have a level path, right?
    if (!gIsOverworld)
    {
        levelGfxDir = normalizePathSlashes(getCustomFolderPath());
    }
    
    // Done figuring out our paths!

    // Read level directory listing
    std::unordered_map<std::wstring, ResourceFileInfo> levelFiles;
    if (levelGfxDir.length() > 0)
    {
        ListResourceFilesFromDir(levelGfxDir.c_str(), levelFiles);
    }

    // Read episode directory listing
    std::unordered_map<std::wstring, ResourceFileInfo> episodeFiles;
    if (episodePath.length() > 0)
    {
        ListResourceFilesFromDir(episodePath.c_str(), episodeFiles);
    }

    // Resolve correct resource file info for each category
    for (int i = 0; smbxImageLoaderCategories[i] != nullptr; i++)
    {
        smbxImageLoaderCategories[i]->resolveResources(
            appGfxDir, episodeGfxDir, levelFiles, episodeFiles, *foundResources
            );
    }

    // Load each normal category's GFX
    for (int i = 0; smbxImageLoaderCategories[i] != nullptr; i++)
    {
        smbxImageLoaderCategories[i]->updateLoadedImages(foundResources, lastResources);
    }

    // Read 'hardcoded' GFX
    static bool loadedHardcoded = false; // TODO: Won't need this odd 'loadedHardcoded' shenanigans when the initial run of ImageLoader::Run is moved to after GM_FORM_GFX is initialized
    if (GM_FORM_GFX != nullptr)
    {
        ResolveHardcodedGfx(appGfxDir, episodeGfxDir, levelFiles, episodeFiles, *foundResources);
        LoadHardcodedGfx(foundResources, loadedHardcoded ? lastResources : nullptr);
        loadedHardcoded = true;
    }

    if (initialLoad)
    {
        for (int i = 0; i < smbxImageCategoryBackground.getArrayLength(); i++)
        {
            GM_GFX_BACKGROUND_W_UNK_PTR[i] = GM_GFX_BACKGROUND_W_PTR[i];
            GM_GFX_BACKGROUND_H_UNK_PTR[i] = GM_GFX_BACKGROUND_H_PTR[i];
        }
    }

    for (int i = 0; i < smbxImageCategoryBlock.getArrayLength(); i++)
    {
        GM_GFX_BLOCKS_NO_MASK[i] = (smbxImageCategoryBlock.getMaskPtr(i + smbxImageCategoryBlock.getFirstIdx()) == nullptr) ? -1 : 0;
    }

    delete lastResources;
    lastResources = foundResources;
}

void ImageLoader::ResolveHardcodedGfx(const std::wstring& appGfxDir, const std::wstring& episodeGfxDir, const std::unordered_map<std::wstring, ResourceFileInfo>& levelFiles, const std::unordered_map<std::wstring, ResourceFileInfo>& episodeFiles, std::unordered_map<std::wstring, ResourceFileInfo>& outData)
{
    std::wstring appGfxTypeDir = (appGfxDir.length() > 0) ? appGfxDir + L"/hardcoded" : L"";
    std::wstring episodeGfxTypeDir = (episodeGfxDir.length() > 0) ? episodeGfxDir + L"/hardcoded" : L"";

    for (int idx1 = 1; idx1 <= HardcodedGraphicsItem::Size(); idx1++)
    {
        HardcodedGraphicsItem& hItemInfo = HardcodedGraphicsItem::Get(idx1);

        // No processing invalid or mask items here
        if ((hItemInfo.state != HardcodedGraphicsItem::HITEMSTATE_NORMAL) && (hItemInfo.state != HardcodedGraphicsItem::HITEMSTATE_ARRAY))
            continue;

        int minItem = hItemInfo.isArray() ? hItemInfo.minItem : -1;
        int maxItem = hItemInfo.isArray() ? hItemInfo.maxItem : -1;
        for (int idx2 = minItem; idx2 <= maxItem; idx2++)
        {
            if (hItemInfo.isArray() && !hItemInfo.isValidArrayIndex(idx2))
            {
                // If this index isn't valid, skip it
                continue;
            }

            std::wstring hardcodedName = L"hardcoded-" + std::to_wstring(idx1);
            if (hItemInfo.isArray())
            {
                hardcodedName += L"-" + std::to_wstring(idx2);
            }

            resolveImageResource(appGfxTypeDir, episodeGfxTypeDir, hardcodedName, levelFiles, episodeFiles, outData);
        }
    }
}

void ImageLoader::LoadHardcodedGfx(const std::unordered_map<std::wstring, ResourceFileInfo>* fileData, const std::unordered_map<std::wstring, ResourceFileInfo>* oldFileData)
{
    for (int idx1 = 1; idx1 <= HardcodedGraphicsItem::Size(); idx1++)
    {
        HardcodedGraphicsItem& hItemInfo = HardcodedGraphicsItem::Get(idx1);
        
        // No processing invalid or mask items here
        if ((hItemInfo.state != HardcodedGraphicsItem::HITEMSTATE_NORMAL) && (hItemInfo.state != HardcodedGraphicsItem::HITEMSTATE_ARRAY))
            continue;

        int minItem = hItemInfo.isArray() ? hItemInfo.minItem : -1;
        int maxItem = hItemInfo.isArray() ? hItemInfo.maxItem : -1;
        for (int idx2 = minItem; idx2 <= maxItem; idx2++)
        {
            if (hItemInfo.isArray() && !hItemInfo.isValidArrayIndex(idx2))
            {
                // If this index isn't valid, skip it
                continue;
            }
            HDC colorHDC = nullptr;
            HDC maskHDC = nullptr;
            hItemInfo.getHDC(idx2, &colorHDC, &maskHDC);

            if (colorHDC == nullptr)
            {
                // No such thing as mask-only graphics
                continue;
            }

            std::wstring hardcodedName = L"hardcoded-" + std::to_wstring(idx1);
            if (hItemInfo.isArray())
            {
                hardcodedName +=  L"-" + std::to_wstring(idx2);
            }

            // Make note of name to HDC mapping
            m_NameToHDC[WStr2Str(hardcodedName)] = (uintptr_t)colorHDC;

            ResourceFileInfo newMain, newMask;
            ResourceFileInfo oldMain, oldMask;
            getImageResource(hardcodedName, *fileData, newMain, newMask);
            if (oldFileData != nullptr)
            {
                getImageResource(hardcodedName, *oldFileData, oldMain, oldMask);
            }

            // If anything has changed, reload
            if ((oldFileData == nullptr) || (newMain != oldMain) || (newMask != oldMask))
            {
                std::shared_ptr<LunaImage> img = nullptr;
                std::shared_ptr<LunaImage> mask = nullptr;

                if (newMain.done)
                {
                    // If we found a file, load from it
                    img = LunaImage::fromFile(newMain.path.c_str());
                    mask = LunaImage::fromFile(newMask.path.c_str());
                }
                else
                {
                    // Otherwise, load from HDC
                    img = LunaImage::fromHDC(colorHDC);
                    mask = LunaImage::fromHDC(maskHDC);

                    // TODO: Consider special case for maskless hardcoded graphics from HDC, where black should be made transparent
                }

                if (img && mask)
                {
                    img->setMask(mask);
                }

                if (img)
                {
                    if (colorHDC != nullptr) ImageLoader::m_Gfx[(uintptr_t)colorHDC] = img;
                    if (maskHDC != nullptr)  ImageLoader::m_Gfx[(uintptr_t)maskHDC] = img;
                }
            }
        }
    }
}

std::shared_ptr<LunaImage> ImageLoader::GetByHDC(HDC hdc, bool bypassOverride) {
    if (!bypassOverride)
    {
        auto it = m_GfxOverride.find((uintptr_t)hdc);
        if (it != m_GfxOverride.end())
        {
            return it->second;
        }
    }

    auto it = m_Gfx.find((uintptr_t)hdc);
    if (it != m_Gfx.end())
    {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<LunaImage> ImageLoader::GetCharacterSprite(short charId, short powerup)
{
    HDC* mainArray = nullptr;
    HDC mainHdc = nullptr;

    // Sanity check
    if (powerup < 1 || powerup > 10) return nullptr;

    switch (charId)
    {
    case 1:
        mainArray = GM_GFX_MARIO_PTR;
        break;
    case 2:
        mainArray = GM_GFX_LUIGI_PTR;
        break;
    case 3:
        mainArray = GM_GFX_PEACH_PTR;
        break;
    case 4:
        mainArray = GM_GFX_TOAD_PTR;
        break;
    case 5:
        mainArray = GM_GFX_LINK_PTR;
        break;
    default:
        // TODO: Support custom characters from this call
        return nullptr;
    }

    mainHdc = (mainArray != nullptr) ? mainArray[powerup - 1] : nullptr;
    if (mainHdc == nullptr) return nullptr;

    return GetByHDC(mainHdc);
}

void ImageLoader::RegisterExtraGfx(const std::string& folderName, const std::string& name)
{
    if (name.length() == 0) return;

    std::wstring wFolderName = Str2WStr(folderName);
    std::wstring wName = Str2WStr(name);
    std::vector<std::wstring> searchPath;
    if (!gIsOverworld)
        searchPath.push_back(normalizePathSlashes(getCustomFolderPath())); // Check custom folder
    searchPath.push_back(normalizePathSlashes(GM_FULLDIR)); // Check episode dir
    searchPath.push_back(normalizePathSlashes(GM_FULLDIR) + L"/graphics/" + wFolderName); // Check episode dir
    searchPath.push_back(normalizePathSlashes(gAppPathWCHAR) + L"/graphics/" + wFolderName); // Check base game

    std::shared_ptr<LunaImage> img = nullptr;
    for (auto pathIt = searchPath.cbegin(); pathIt != searchPath.cend(); pathIt++)
    {
        std::wstring imgPath = *pathIt + L"/" + wName + L".png";
        img = LunaImage::fromFile(imgPath.c_str());
        if (img)
        {
            break;
        }
    }
    
    // Add to map, even if null
    m_ExtraGfx[name] = img;
}

void ImageLoader::UnregisterExtraGfx(const std::string& name)
{
    m_ExtraGfx.erase(name);
    m_ExtraGfxOverride.erase(name);
}

std::shared_ptr<LunaImage> ImageLoader::GetByName(const std::string& name, bool bypassOverride)
{
    // Get image for the normal case where we're mapping through HDCs
    {
        auto it = m_NameToHDC.find(name);
        if (it != m_NameToHDC.end()) {
            return GetByHDC((HDC)it->second, bypassOverride);
        }
    }

    // Handle returning "extra" gfx
    {
        auto it = m_ExtraGfx.find(name);
        if (it != m_ExtraGfx.end())
        {
            if (!bypassOverride)
            {
                auto itOverride = m_ExtraGfxOverride.find(name);
                if (itOverride != m_ExtraGfxOverride.end())
                {
                    return itOverride->second;
                }
            }

            return it->second;
        }
    }

    return nullptr;
}

bool ImageLoader::OverrideByName(const std::string& name, const std::shared_ptr<LunaImage>& img)
{
    // If we're mapping through an HDC, find it for this image name
    {
        auto it = m_NameToHDC.find(name);
        if (it != m_NameToHDC.end()) {
            if (img)
            {
                m_GfxOverride[it->second] = img;
            }
            else
            {
                m_GfxOverride.erase(it->second);
            }

            return true;
        }
    }

    // Otherwise, for "extra gfx" we're mapping directly from name to image, so set the override that way
    if (m_ExtraGfx.find(name) != m_ExtraGfx.end())
    {
        if (img)
        {
            m_ExtraGfxOverride[name] = img;
        }
        else
        {
            m_ExtraGfxOverride.erase(name);
        }

        return true;
    }

    return false;
}

void ImageLoader::ClearOverrides()
{
    m_GfxOverride.clear();
    m_ExtraGfxOverride.clear();
}
