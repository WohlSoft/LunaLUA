#include "../LuaProxy.h"

#include "../LuaProxy.h"
#include "../../SMBXInternal/Musicbox.h"

unsigned short LuaProxy::Musicbox::count()
{
    return ::SMBXMusicbox::Count();
}

luabind::object LuaProxy::Musicbox::get(lua_State* L)
{
    return LuaHelper::getObjList(::SMBXMusicbox::Count(), [](unsigned short i) { return LuaProxy::Musicbox(i); }, L);
}

// TODO: Consider if there's a good way to use C++ templates to make it so
//       entity id filtering code isn't duplicated.
luabind::object LuaProxy::Musicbox::get(luabind::object idFilter, lua_State* L)
{
    std::unique_ptr<bool> lookupTableMusicboxID;

    try
    {
        lookupTableMusicboxID = std::unique_ptr<bool>(LuaHelper::generateFilterTable(L, idFilter, ::SMBXMusicbox::MAX_ID));
    }
    catch (LuaHelper::invalidIDException* e)
    {
        luaL_error(L, "Invalid Musicbox-ID!\nNeed Musicbox-ID between 1-%d\nGot Musicbox-ID: %d", ::SMBXMusicbox::MAX_ID, e->usedID());
        return luabind::object();
    }
    catch (LuaHelper::invalidTypeException* /*e*/)
    {
        luaL_error(L, "Invalid args for musicboxID (arg #1, expected table or number, got %s)", lua_typename(L, luabind::type(idFilter)));
        return luabind::object();
    }

    return LuaHelper::getObjList(
        ::SMBXMusicbox::Count(),
        [](unsigned short i) { return LuaProxy::Musicbox(i); },
        [&lookupTableMusicboxID](unsigned short i) {
        ::SMBXMusicbox *bgo = ::SMBXMusicbox::Get(i);
        return (bgo != NULL) &&
            (bgo->id <= ::SMBXMusicbox::MAX_ID) && lookupTableMusicboxID.get()[bgo->id];
    }, L);
}

LuaProxy::Musicbox::Musicbox(unsigned short index)
{
    m_index = index;
}

int LuaProxy::Musicbox::idx() const
{
    return m_index;
}

luabind::object LuaProxy::Musicbox::getIntersecting(double x1, double y1, double x2, double y2, lua_State* L)
{
    return LuaHelper::getObjList(
        ::SMBXMusicbox::Count(),
        [](unsigned short i) { return LuaProxy::Musicbox(i); },
        [x1, y1, x2, y2](unsigned short i) {
        ::SMBXMusicbox *obj = ::SMBXMusicbox::Get(i);
        if (obj == NULL) return false;
        if (x2 <= obj->momentum.x) return false;
        if (y2 <= obj->momentum.y) return false;
        if (obj->momentum.x + obj->momentum.width <= x1) return false;
        if (obj->momentum.y + obj->momentum.height <= y1) return false;
        return true;
    }, L);
}

short LuaProxy::Musicbox::id(lua_State* L) const
{
    if (!isValid_throw(L))
        return 0;
    return SMBXMusicbox::Get(m_index)->id;
}

void LuaProxy::Musicbox::setId(short id, lua_State* L)
{
    if (!isValid_throw(L))
        return;
    SMBXMusicbox::Get(m_index)->id = id;
}

double LuaProxy::Musicbox::x(lua_State* L) const
{
    if (!isValid_throw(L))
        return 0;
    return SMBXMusicbox::Get(m_index)->momentum.x;
}

void LuaProxy::Musicbox::setX(double x, lua_State* L) const
{
    if (!isValid_throw(L))
        return;
    SMBXMusicbox::Get(m_index)->momentum.x = x;
}

double LuaProxy::Musicbox::y(lua_State* L) const
{
    if (!isValid_throw(L))
        return 0;
    return SMBXMusicbox::Get(m_index)->momentum.y;
}

void LuaProxy::Musicbox::setY(double y, lua_State* L) const
{
    if (!isValid_throw(L))
        return;
    SMBXMusicbox::Get(m_index)->momentum.y = y;
}

double LuaProxy::Musicbox::width(lua_State* L) const
{
    if (!isValid_throw(L))
        return 0;
    return SMBXMusicbox::Get(m_index)->momentum.width;
}

void LuaProxy::Musicbox::setWidth(double width, lua_State* L)
{
    if (!isValid_throw(L))
        return;
    SMBXMusicbox::Get(m_index)->momentum.width = width;
}

double LuaProxy::Musicbox::height(lua_State* L) const
{
    if (!isValid_throw(L))
        return 0;
    return SMBXMusicbox::Get(m_index)->momentum.height;
}

void LuaProxy::Musicbox::setHeight(double height, lua_State* L)
{
    if (!isValid_throw(L))
        return;
    SMBXMusicbox::Get(m_index)->momentum.height = height;
}

bool LuaProxy::Musicbox::isValid() const
{
    return (m_index < ::SMBXMusicbox::Count());
}

bool LuaProxy::Musicbox::isValid_throw(lua_State *L) const
{
    if (!isValid()) {
        luaL_error(L, "Invalid Musicbox-Pointer");
        return false;
    }
    return true;
}
