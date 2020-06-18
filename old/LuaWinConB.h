/* * * * * * * * * * * * * * * * *
 *     Created February 2012     *
 *  by Matthew Britton Sessions  *
 *     Designed for Lua 5.2      *
 * * * * * * * * * * * * * * * * */

#include <lua.h>
#include <lauxlib.h>

#include <windows.h>

/* EXPORTED API FUNCTIONS */
static int lw32c_SetTitle(lua_State *L);
static int lw32c_SetBufferDimensions(lua_State *L);
static int lw32c_SetWindowPosition(lua_State *L);
static int lw32c_MakeActiveBuffer(lua_State *L);
static int lw32c_CreateNewBuffer(lua_State *L);
static int lw32c_GetLargestWindowDimensions(lua_State *L);

static int lw32c_SetInputEnabled(lua_State *L);
static int lw32c_ClearInput(lua_State *L);
static int lw32c_ReadInput(lua_State *L);
static int lw32c_PeekInput(lua_State *L);

static int lw32c_SetCursorVisible(lua_State *L);
static int lw32c_SetCursorPosition(lua_State *L);
static int lw32c_SetCursorSize(lua_State *L);

static int lw32c_FillRegion(lua_State *L);
static int lw32c_WriteRegion(lua_State *L);
static int lw32c_WriteRun(lua_State *L);
/* END EXPORTED API FUNCTIONS */

/* OPEN FUNCTION */
int luaopen_LuaWinCon(lua_State *L);
/* END OPEN FUNCTION */

/* INTERNAL FUNCTIONS */
static const WCHAR *luaW_tostring(lua_State *L, int index);

static BOOL   lw32ci_IsValidBuffer(lua_State *L, int index);
static BOOL   lw32ci_IsValidCoord(lua_State *L, int index);

static HANDLE lw32ci_ToHandle(lua_State *L, int index);
static COORD  lw32ci_ToCoord(lua_State *L, int index, int based); /* 0 for 0-based coords, 1 for 1-based coords. */

static void   lw32ci_PushHandle(lua_State *L, HANDLE h);
static void   lw32ci_PushCoord(lua_State *L, COORD c, int based);
/* END INTERNAL FUNCTIONS */