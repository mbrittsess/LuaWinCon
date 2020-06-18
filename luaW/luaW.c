/* * * * * * * * * * * * * * 
    Created February 2012
 by Matthew Britton Sessions
    Designed for Lua 5.1
 * * * * * * * * * * * * * */

#include <lua.h>
#include "luaW.h"
#include <windows.h>

#if LUA_VERSION_NUM == 502
 #define str_or_void const char*
#elif LUA_VERSION_NUM == 501
 #define str_or_void void
#endif

LUA_API str_or_void (luaW_pushlstring) (lua_State *L, const WCHAR *s, size_t cch_l) {
    char*       ThisUTF8Str;
    int         ThisUTF8Siz;
    HANDLE      ThisHeap;
    const char* ThisRet;
    
    if (cch_l == 0) {
      #if LUA_VERSION_NUM == 502
        return lua_pushliteral(L, "");
      #elif LUA_VERSION_NUM == 501
        lua_pushliteral(L, "");
        return;
      #endif
    } else {
        ThisUTF8Siz = WideCharToMultiByte(
            CP_UTF8,
            0,
            s,
            cch_l,
            NULL,
            0,
            NULL,
            NULL);
    
        ThisHeap = GetProcessHeap();
        ThisUTF8Str = (char*)HeapAlloc(ThisHeap, 0, ThisUTF8Siz * sizeof(char));
    
        WideCharToMultiByte(
            CP_UTF8,
            0,
            s,
            cch_l,
            ThisUTF8Str,
            ThisUTF8Siz,
            NULL,
            NULL);
    
      #if LUA_VERSION_NUM == 502
        ThisRet = lua_pushlstring(L, ThisUTF8Str, ThisUTF8Siz);
    
        HeapFree(ThisHeap, 0, ThisUTF8Str);
        return ThisRet;
      #elif LUA_VERSION_NUM == 501
        lua_pushlstring(L, ThisUTF8Str, ThisUTF8Siz);
    
        HeapFree(ThisHeap, 0, ThisUTF8Str);
        return;
      #endif
    };
};

LUA_API str_or_void (luaW_pushstring) (lua_State *L, const WCHAR *s) {
    char*       ThisUTF8Str;
    int         ThisUTF8Siz;
    HANDLE      ThisHeap = GetProcessHeap();
    const char* ThisRet;
    
    ThisUTF8Siz = WideCharToMultiByte(
        CP_UTF8,
        0,
        s,
        -1,
        NULL,
        0,
        NULL,
        NULL);
    
    ThisUTF8Str = (char*)HeapAlloc(ThisHeap, 0, ThisUTF8Siz * sizeof(char));
    
    WideCharToMultiByte(
        CP_UTF8,
        0,
        s,
        -1,
        ThisUTF8Str,
        ThisUTF8Siz,
        NULL,
        NULL);
    
  #if LUA_VERSION_NUM == 502
    ThisRet = lua_pushlstring(L, ThisUTF8Str, ThisUTF8Siz);
    
    HeapFree(ThisHeap, 0, ThisUTF8Str);
    return ThisRet;
  #elif LUA_VERSION_NUM == 501
    lua_pushlstring(L, ThisUTF8Str, ThisUTF8Siz);
    
    HeapFree(ThisHeap, 0, ThisUTF8Str);
    return;
  #endif
};

LUA_API const WCHAR *(luaW_tolstring) (lua_State *L, int idx, size_t *cch_len) {
    /* This function is not precisely an analogue of lua_tolstring(). Whereas that function returns
    a pointer to the internal state of the string at the given index, this one pushes that such string,
    converted to UTF-16, onto the top of the stack and returns a pointer to *that*. 
       If the value at 'idx' is not a string or cannot be converted to a string, the function returns
    null and nothing is pushed. */
    const char*  ThisUTF8Str;
    size_t       ThisUTF8Siz;
    WCHAR*       ThisUTF16Buf;
    size_t       ThisUTF16BufCchSiz;
    const WCHAR* ThisUTF16Internal;
    size_t       ThisUTF16InternalCbSiz;
    HANDLE       ThisHeap = GetProcessHeap();
    
    ThisUTF8Str = lua_tolstring(L, idx, &ThisUTF8Siz);
    if (ThisUTF8Str == NULL) {
        return NULL;
    };
    
    ThisUTF16BufCchSiz = MultiByteToWideChar(
        CP_UTF8,
        0,
        ThisUTF8Str,
        ThisUTF8Siz,
        NULL,
        0);
    
    ThisUTF16Buf = (LPWSTR)HeapAlloc(ThisHeap, 0, ThisUTF16BufCchSiz * sizeof(WCHAR));
    MultiByteToWideChar(
        CP_UTF8,
        0,
        ThisUTF8Str,
        ThisUTF8Siz,
        ThisUTF16Buf,
        ThisUTF16BufCchSiz);
    
    lua_pushlstring(L, (const char*)ThisUTF16Buf, ThisUTF16BufCchSiz * sizeof(WCHAR));
    
    HeapFree(ThisHeap, 0, ThisUTF16Buf);
    
    ThisUTF16Internal = (LPCWSTR)lua_tolstring(L, -1, &ThisUTF16InternalCbSiz);
    if (cch_len != NULL) {
        *cch_len = ThisUTF16InternalCbSiz / sizeof(WCHAR);
    };
    
    return ThisUTF16Internal;
};