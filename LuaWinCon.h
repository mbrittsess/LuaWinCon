/* INCLUDES */
#include <lua.h>
#include <lauxlib.h>
#include <windows.h>
#include "vk_definitions.h"
#include "coord_meta.h"

/* LISTS, DEFINITIONS */
#define UPV_RootTable lua_upvalueindex(1)
#define UPV_KeyCodeTranslationTable lua_upvalueindex(2)
#define NumberOfUpvalues 2 /* Tried being smart about defining this and it failed miserably. */
/*
#undef UPVStart
#undef UPVEnd
*/

enum lwc_RootEntries {
    #define RootEntryStart __LINE__
    RE_OriginalSTDIN = 1,    /* Light Userdata, literal file handle */
    RE_OriginalSTDOUT,       /* Light Userdata, literal file handle */
    RE_OriginalSTDERR,       /* Light Userdata, literal file handle */
    RE_CurrentInputBuf,      /* Light Userdata, literal console input buffer handle */
    RE_DefaultScreenBuf,     /* Full Userdata, ScreenBufObj */
    RE_ScreenBufMetaTable,   /* Lua Table */
    RE_ScreenBufMethodTable, /* Lua Table */
    RE_CoordMetaTable,       /* Lua Table */
    RE_DefaultColorTbl,      /* Lua Table */
    RE_ColorTblLookup,       /* Lua Table */
    RE_DefaultAttrib         /* Lua Table */
    #define RootEntryEnd __LINE__
}; 
#define NumberOfRootEntries ((29 - 19) + 1) /* It's getting ridiculous, what I have to go through to get this thing to work. Might as well
    do it manually. */
/*#define NumberOfRootEntries (RootEntryEnd - RootEntryStart - 1)*/
/*static int NumberOfRootEntries = RootEntryEnd - RootEntryStart - 1;*/
/*
#undef RootEntryStart
#undef RootEntryEnd
*/

typedef struct { /* We will represent this with a structure so that we can easily expand it later if we so desire. */
    HANDLE Handle;
} ScreenBufObj;

#define NumArgsErrString "Invalid number of arguments to function '%s', please pass exactly %d arguments!"
#define NumArgsRangeErrString "Invalid number of arguments to function '%s', please pass from %d to %d arguments!"
#define NotAScreenBufferErrString "Invalid argument #%d to function '%s': not a screenbuffer!"
#define NotAStringOrConvertibleErrString "Invalid argument #%d to function '%s': not a string or convertible to a string!"
#define NotANumberOrConvertibleErrString "Invalid argument #%d to function '%s': not a number or convertible to a number!"
#define NotACoordErrString "Invalid argument #%d to function '%s': not a coord!"
#define NotAPositionCoordErrString "Invalid argument #%d to function '%s': not a position coord!"
#define NotASizeCoordErrString "Invalid argument #%d to function '%s': not a size coord!"
#define NotAnAttribErrString "Invalid argument #%d to function '%s': not an attrib!"
#define NotAColorArrayErrString "Invalid argument #%d to function '%s': not a colorarray!"

#if LUA_VERSION_NUM == 502
/* Defined here for compatibility. */
#define lua_equal(L, index1, index2) lua_compare(L, index1, index2, LUA_OPEQ)
#endif

#if LUA_VERSION_NUM == 501
#define lua_rawlen(L, idx) lua_objlen(L, idx)
#endif

typedef struct {
    enum {RT_SENTRY, RT_CFUNC, RT_LFUNC} Type;
    union {
        lua_CFunction CFunc;
        const char*   LFunc; 
        /* The chunk created from the pointed-to string in LFunc should return the actual value it contains, i.e.:
            LFunc = "return function(self, position) print(self.area, position) end"; */
    } F;
    const char* Name;
} lwc_Reg;

lwc_Reg ExportedFuncs[];
lwc_Reg ScreenBufMethods[];

/*#define Reg_Entry( type, func ) {type, prefix##func, #func}*/

/* FUNCTION DECLARATIONS */
/* Library Init Funcs */
 int luaopen_LuawinCon(lua_State *L);
 static void lwci_MakeScreenBufMetaTable(lua_State *L);
 static void lwci_MakeScreenBufMethodTable(lua_State *L);
 static void lwci_MakeCoordMetaTable(lua_State *L);
 static void lwci_PushUpvalues(lua_State *L);
 static void lwci_DoRunTimeLinking();
/* Exported Library Funcs */
 /* Uncategorized */
  static int lwcl_NewConsole(lua_State *L);
  static int lwcl_DefaultBuffer(lua_State *L);
  static int lwcl_GetNewBuffer(lua_State *L);
  static int lwcl_SetTitle(lua_State *L);
  static int lwcl_LowLevelMode(lua_State *L);
  static int lwcl_MakeCoord(lua_State *L);
  static int lwcl_SetDefaultColorTable(lua_State *L);
  static int lwcl_GetDefaultAttrib(lua_State *L);
 /* Input Funcs */
  static int lwcl_ClearInput(lua_State *L);
  static int lwcl_IsInputAvailable(lua_State *L);
  static int lwcl_FetchInput(lua_State *L);
/* Library Utility Funcs */
 /* Uncategorized */
  static void lwcu_Register(lua_State *L, lwc_Reg list[]);
  static int lwcu_AttemptToIndex(lua_State *L);
 /* Coord Funcs */
  static int lwcu_IsACoord(lua_State *L, int idx);
  static int lwcu_IsAPositionCoord(lua_State *L, int idx);
  static int lwcu_IsASizeCoord(lua_State *L, int idx);
  static void lwcu_PushCoord(lua_State *L, COORD C, signed int Adjustment);
  static COORD lwcu_ToCoord(lua_State *L, int idx, signed int Adjustment);
 /* Attrib Funcs */
  static int lwcu_IsAnAttrib(lua_State *L, int idx);
  static WORD lwcu_ToAttrib(lua_State *L, int idx);
  static void lwcu_PushAttrib(lua_State *L, WORD attr);
 /* ScreenBuffer Funcs */
  static int lwcu_IsAScreenBuffer(lua_State *L, int idx);
  static void lwcu_PushScreenBufObjWithinLuaOpen(lua_State *L, HANDLE ThisHandle);
  static void lwcu_PushScreenBufObj(lua_State *L, HANDLE ThisHandle);
 /* Input Record Funcs */
  static void lwcu_PushKeyRecord(lua_State *L, KEY_EVENT_RECORD *K);
  static void lwcu_PushMouseRecord(lua_State *L, MOUSE_EVENT_RECORD *M);
 /* Color Array Funcs */
  static void lwcu_PushColorArray(lua_State *L, COLORREF color_tbl[]);
  static void lwcu_ToColorArray(lua_State *L, int idx, COLORREF color_tbl[]);
  static int lwcu_IsAColorArray(lua_State *L, int idx);
/* ScreenBuffer Methods */
 static int lwcsbm_Close(lua_State *L);
 static int lwcsbm_MakeActive(lua_State *L);
 static int lwcsbm_GetLargestWindowSize(lua_State *L);
 static int lwcsbm_SetCursorVisible(lua_State *L);
 static int lwcsbm_SetCursorPosition(lua_State *L);
 static int lwcsbm_SetCursorSize(lua_State *L);
 static int lwcsbm_SetScreenBufferSize(lua_State *L);
 static int lwcsbm_SetWindowPositionAndSize(lua_State *L);
 static int lwcsbm_FillRegion(lua_State *L);
 static int lwcsbm_WriteRegion(lua_State *L);
 static int lwcsbm_WriteRun(lua_State *L);
 static int lwcsbm_SetColorTable(lua_State *L);
 #if 0
 static int lwcsbm_GetColorTable(lua_State *L);
 #endif
 static int lwcsbm_gc(lua_State *L);

/* RUNTIME-LINKED FUNCTION DECLARATION AND STORAGE */
typedef BOOL (WINAPI *PGCSBIEx)(HANDLE, PCONSOLE_SCREEN_BUFFER_INFOEX);
typedef BOOL (WINAPI *PSCSBIEx)(HANDLE, PCONSOLE_SCREEN_BUFFER_INFOEX);
#define GetConsoleScreenBufferInfoEx pfnGetConsoleScreenBufferInfoEx
#define SetConsoleScreenBufferInfoEx pfnSetConsoleScreenBufferInfoEx
static PGCSBIEx GetConsoleScreenBufferInfoEx = NULL;
static PSCSBIEx SetConsoleScreenBufferInfoEx = NULL;
#define ColorFuncsAreAvailable ((GetConsoleScreenBufferInfoEx != NULL) && (SetConsoleScreenBufferInfoEx != NULL))