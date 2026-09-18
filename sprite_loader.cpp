extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}

#include <cstdio>
#include <cstdint>
#include <cstdlib>

// 和打包器结构体严格一致、对齐一致
#pragma pack(push, 1)
struct SpriteEntry
{
    uint32_t width;
    uint32_t height;
    uint32_t off_x;
    uint32_t off_y;
    uint64_t offset;
    uint32_t data_size;
    uint32_t format;
};
#pragma pack(pop)

static int l_loadHeader(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);
    FILE* f = fopen(path, "rb");
    if (!f)
    {
        return luaL_error(L, "cannot open spk file");
    }

    uint32_t total_img;
    fread(&total_img, 4, 1, f);

    SpriteEntry* entries = (SpriteEntry*)malloc(sizeof(SpriteEntry) * total_img);
    fread(entries, sizeof(SpriteEntry), total_img, f);
    fclose(f);

    lua_newtable(L);
    lua_pushinteger(L, total_img);
    lua_setfield(L, -2, "count");

    lua_newtable(L);
    for (uint32_t i = 0; i < total_img; ++i)
    {
        lua_newtable(L);
        lua_pushinteger(L, entries[i].width);
        lua_setfield(L, -2, "w");
        lua_pushinteger(L, entries[i].height);
        lua_setfield(L, -2, "h");
        lua_pushinteger(L, entries[i].off_x);
        lua_setfield(L, -2, "off_x");
        lua_pushinteger(L, entries[i].off_y);
        lua_setfield(L, -2, "off_y");
        lua_pushinteger(L, entries[i].offset);
        lua_setfield(L, -2, "offset");
        lua_pushinteger(L, entries[i].data_size);
        lua_setfield(L, -2, "size");
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "list");

    free(entries);
    return 1;
}

static const luaL_Reg lib[] = {
    {"loadHeader", l_loadHeader},
    {NULL, NULL}
};

extern "C" int luaopen_sprite_loader(lua_State* L)
{
    luaL_register(L, "sprite_loader", lib);
    return 1;
}