/* * * * * * * * * * * * * * 
    Created February 2012
 by Matthew Britton Sessions
    Designed for Lua 5.2
 * * * * * * * * * * * * * */

#include <windows.h>
#include <lua.h>

#if LUA_VERSION_NUM == 502

LUA_API const char *(luaW_pushlstring)(lua_State *L, const WCHAR *s, size_t cch_l);
LUA_API const char *(luaW_pushstring)(lua_State *L, const WCHAR *s);
#define luaW_pushliteral(M, s) \
    luaW_pushlstring(M, L"" s, (sizeof(s)/sizeof(WCHAR))-1)
    
LUA_API const WCHAR *luaW_tolstring(lua_State *L, int idx, size_t *cch_len);
#define luaW_tostring(L,i) luaW_tolstring(L, (i), NULL)

#elif LUA_VERSION_NUM == 501

LUA_API void (luaW_pushlstring)(lua_State *L, const WCHAR *s, size_t cch_l);
LUA_API void (luaW_pushstring)(lua_State *L, const WCHAR *s);
#define luaW_pushliteral(M, s) \
    luaW_pushlstring(M, L"" s, (sizeof(s)/sizeof(WCHAR))-1)
    
LUA_API const WCHAR *luaW_tolstring(lua_State *L, int idx, size_t *cch_len);
#define luaW_tostring(L,i) luaW_tolstring(L, (i), NULL)

#endif