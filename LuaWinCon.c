/* * * * * * * * * * * * * * 
    Created February 2012
 by Matthew Britton Sessions
Designed for Lua 5.1 and 5.2

 Best viewed at 140 columns.
 * * * * * * * * * * * * * */
 
/* * * * * * * * * * * * * * * * *
        TABLE OF CONTENTS
    
  Ctrl+f for the given string to
jump to each indicated section.

Includes . . . . . . . . SEC_INC
Library Init Func. . . . SEC_INIT
Exported Library Funcs . SEC_LIB
  Uncategorized. . . . . . SECSUB_LIB_UNCAT
  Input Functions. . . . . SECSUB_LIB_INPUT
Library Utility Funcs. . SEC_UTIL
  Uncategorized. . . . . . SECSUB_UTIL_UNCAT
  Coord Functions. . . . . SECSUB_UTIL_CORD
  Attrib Functions . . . . SECSUB_UTIL_ATTR
  ScreenBuffer Functions . SECSUB_UTIL_SCRB
  Input Record Functions . SECSUB_UTIL_INRC
  ColorArray Functions . . SECSUB_UTIL_COLARR
Screenbuffer Methods . . SEC_METHS
 * * * * * * * * * * * * * * * * */

/* Here for debugging purposes. */
#define lua_print(L, str) \
    lua_getglobal(L, "print"); \
    lua_pushstring(L, str); \
    lua_call(L, 1, 0)

/* SEC_INC */
#include "LuaWinCon.h"
#include "luaW.h"

/* SEC_INIT */
static int RootTableStackIdx; /* Since the RootTable isn't accessible as an upvalue from luaopen_LuaWinCon, we will store its
    stack index here, and any function which needs to work with this table which is guaranteed to only be called from within
    luaopen_LuaWinCon will refer to its index with the value in this variable. */

int luaopen_LuaWinCon(lua_State *L) {
    
    struct {
        HANDLE StandardInput;
        HANDLE StandardOutput;
        HANDLE StandardError;
    } OriginalHandles;
    
    /* Very first thing we're going to do upon starting is to prepare our RootTable, the single shared table which goes into
    the upvalues of every function in this library. */
    lua_createtable(L, NumberOfRootEntries, 0);
    RootTableStackIdx = lua_gettop(L);
    
    /* Next, we'll prepare ourselves a console for use. This may involve creating a new one...allocating a new console
    overwrites the STDIN, STDOUT, and STDERR values returned by GetStdHandle. Whether this is a process-wide effect or not,
    we're going to play it safe and save these handles first. */
    
    OriginalHandles.StandardInput  = GetStdHandle(STD_INPUT_HANDLE);
    OriginalHandles.StandardOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    OriginalHandles.StandardError  = GetStdHandle(STD_ERROR_HANDLE);
    
    /* We'll save them to some upvalues. They aren't used at the moment, but we could very well use them in the future. */
    lua_pushlightuserdata(L, OriginalHandles.StandardInput);
    lua_rawseti(L, RootTableStackIdx, RE_OriginalSTDIN);
    
    lua_pushlightuserdata(L, OriginalHandles.StandardOutput);
    lua_rawseti(L, RootTableStackIdx, RE_OriginalSTDOUT);
    
    lua_pushlightuserdata(L, OriginalHandles.StandardError);
    lua_rawseti(L, RootTableStackIdx, RE_OriginalSTDERR);
    
    /* This next step will either fetch the handle to our default console and input buffer, or create some if we don't have
    any, and then restore our standard handles (which would have been overwritten had we in fact created a new console).
       This step produces the same results no matter the case of whether we had a console or not. It must also be noted that
    this occurs as soon as the library is loaded; there is no manual initialization (unless you want to definitely create a
    new console. */
    
    AllocConsole(); /* Fails if the process is already attached to one. */
    AttachConsole(GetCurrentProcessId());
    
    lwci_MakeScreenBufMetaTable(L); /* Must call this once before creating any ScreenBufObjs. */
    lwci_MakeCoordMetaTable(L);
    
    lwci_DoRunTimeLinking(L); /* Placed here essentially randomly. It can really go anywhere. */
    
    lua_pushlightuserdata(L, CreateFileW(
        L"CONIN$",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL));
    lua_rawseti(L, RootTableStackIdx, RE_CurrentInputBuf);
    
    lwcu_PushScreenBufObjWithinLuaOpen(L, CreateFileW(
        L"CONOUT$",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL));
    lua_pushvalue(L, -1);
    lua_rawseti(L, RootTableStackIdx, RE_DefaultScreenBuf);
    
    {CONSOLE_SCREEN_BUFFER_INFO ThisInfo;
     
     GetConsoleScreenBufferInfo(
        ((ScreenBufObj*)lua_touserdata(L, -1))->Handle,
        &ThisInfo);
     lwcu_PushAttrib(L, ThisInfo.wAttributes);
     lua_rawseti(L, RootTableStackIdx, RE_DefaultAttrib);
    };
    
    /* Here we'll take care of any color-related stuff in our initialization. */
    /* It bears some explanation as to what we're doing with color-related stuff in this library, as it's the one thing where
    the code is not a relatively simply mapping to the appropriate Win32 functions.
       Vista allows you to get and set the "Color Table" for screenbuffers, at the same time as many of their other attributes,
    such as buffer size, window position, cursor visibility, etc. etc. However, this gives the false impression that screen
    buffers carry their own private color tables just as they carry their own private sizes, text data, and cursor positions, etc.
       The reality is more complex, and not documented. From my experiments and observations on a 64-bit Vista Ultimate platform,
    it would appear to be the case that there is only one global color table used for current rendering. When one "sets" the color
    table for a buffer handle associated with the current console, the change in rendered colors will take place instantly if the
    handle used is the same as the current active handle. However, if the handle used is different, then the change will take place
    the next time the active screen buffer is changed, regardless of which buffer it is changed to.
       So, to get the desired behavior, where every buffer appears to carry its own private color table, we have a weak table acceessible
    from our library's internal functions, which has buffer objects as keys and color tables as values. Every buffer object made is
    recorded in there, and upon creation, it will be given the "Default Color Table", which is initialized with the color table of the
    default initial screenbuffer when the library is loaded or LuaWinCon.NewConsole() is called (this value can be changed with a call
    to the new function LuaWinCon.SetDefaultColorTable().)
       Whenever a call is made to the :MakeActive() method of a screenbuffer, a matching call is made to SetConsoleScreenBufferInfoEx()
    using the color table found in our lookup table, using the soon-to-be-active buffer object as a key, and then the screenbuffer
    is made active.
       In this way, the users get the expected behavior, where they can call :SetColorTable() on any screenbuffer, with any color table,
    and the console window will switch to that color scheme whenever that buffer is made active. */
    if (ColorFuncsAreAvailable) {
        CONSOLE_SCREEN_BUFFER_INFOEX ThisInfo;
        ThisInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
        
        GetConsoleScreenBufferInfoEx(
            ((ScreenBufObj*)lua_touserdata(L, -1))->Handle,
            &ThisInfo);
        lwcu_PushColorArray(L, ThisInfo.ColorTable);
        lua_rawseti(L, RootTableStackIdx, RE_DefaultColorTbl);
        
        lua_newtable(L); /* The ColorTblLookup table. */
        lua_newtable(L); /* The metatable for the above table. */
        lua_pushliteral(L, "k");
        lua_setfield(L, -2, "__mode");
        lua_setmetatable(L, -2);
        
        /* Now to place the default screenbuffer into the lookup table. */
        lua_pushvalue(L, -2); /* This is the Default ScreenBuffer userdatum. */
        lua_rawgeti(L, RootTableStackIdx, RE_DefaultColorTbl);
        lua_settable(L, -3); /* This puts the default color table into the lookup table, with the screenbuffer as a key, and leaves the
            color table on top of the stack. */
        
        lua_rawseti(L, RootTableStackIdx, RE_ColorTblLookup);
    };
    
    lua_pop(L, 1);
    
    /* Now to restore the standard streams... */
    SetStdHandle(STD_INPUT_HANDLE,  OriginalHandles.StandardInput);
    SetStdHandle(STD_OUTPUT_HANDLE, OriginalHandles.StandardOutput);
    SetStdHandle(STD_ERROR_HANDLE,  OriginalHandles.StandardError);
    
    /* And now, let's make our export table, then return that. */
    lua_newtable(L);
    lwcu_Register(L, ExportedFuncs);
    
    return 1;
};

static lwc_Reg ExportedFuncs[] = {
    #define Reg_Entry( type, func ) {type, lwcl_##func, #func}
    Reg_Entry(RT_CFUNC, NewConsole),
    Reg_Entry(RT_CFUNC, DefaultBuffer),
    Reg_Entry(RT_CFUNC, GetNewBuffer),
    Reg_Entry(RT_CFUNC, SetTitle),
    Reg_Entry(RT_CFUNC, LowLevelMode),
    Reg_Entry(RT_CFUNC, MakeCoord),
    Reg_Entry(RT_CFUNC, SetDefaultColorTable),
    Reg_Entry(RT_CFUNC, GetDefaultAttrib),
    Reg_Entry(RT_CFUNC, ClearInput),
    Reg_Entry(RT_CFUNC, IsInputAvailable),
    Reg_Entry(RT_CFUNC, FetchInput),
    {RT_SENTRY, NULL}
    #undef Reg_Entry
};

static void lwci_MakeScreenBufMetaTable(lua_State *L) {

    lwci_MakeScreenBufMethodTable(L);
    
    lua_createtable(L, 0, 2);
    lua_rawgeti(L, RootTableStackIdx, RE_ScreenBufMethodTable);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, lwcsbm_gc);
    lua_setfield(L, -2, "__gc");
    
    lua_rawseti(L, RootTableStackIdx, RE_ScreenBufMetaTable);
    
    return;
};

static void lwci_MakeScreenBufMethodTable(lua_State *L) { /* Saves the Screenbuffer method table into its appropriate upvalue. */
    lua_newtable(L);
    lwcu_Register(L, ScreenBufMethods);
    lua_rawseti(L, RootTableStackIdx, RE_ScreenBufMethodTable);
    
    return;
};

static lwc_Reg ScreenBufMethods[] = {
    #define Reg_Entry( type, func ) {type, lwcsbm_##func, #func}
    Reg_Entry(RT_CFUNC, Close),
    Reg_Entry(RT_CFUNC, MakeActive),
    Reg_Entry(RT_CFUNC, GetLargestWindowSize),
    Reg_Entry(RT_CFUNC, SetCursorVisible),
    Reg_Entry(RT_CFUNC, SetCursorPosition),
    Reg_Entry(RT_CFUNC, SetCursorSize),
    Reg_Entry(RT_CFUNC, SetScreenBufferSize),
    Reg_Entry(RT_CFUNC, SetWindowPositionAndSize),
    Reg_Entry(RT_CFUNC, FillRegion),
    Reg_Entry(RT_CFUNC, WriteRegion),
    Reg_Entry(RT_CFUNC, WriteRun),
    Reg_Entry(RT_CFUNC, SetColorTable),
    #if 0
    Reg_Entry(RT_CFUNC, GetColorTable),
    #endif
    {RT_SENTRY, NULL}
    #undef Reg_Entry
};

static void lwci_MakeCoordMetaTable(lua_State *L) {
    luaL_loadstring(L, coord_MakeMetaTableString);
    lua_call(L, 0, 1);
    lua_rawseti(L, RootTableStackIdx, RE_CoordMetaTable);

    return;
};

static void lwci_PushUpvalues(lua_State *L) {
    lua_pushvalue(L, RootTableStackIdx);
    luaL_loadstring(L, vk_MakeTableFuncString);
    lua_call(L, 0, 1);

    return;
};

static void lwci_DoRunTimeLinking(lua_State *L) {
    HMODULE hK32 = GetModuleHandleW(L"kernel32.dll");
    
    GetConsoleScreenBufferInfoEx = (PGCSBIEx)GetProcAddress(
        hK32,
        "GetConsoleScreenBufferInfoEx"
    );
    SetConsoleScreenBufferInfoEx = (PSCSBIEx)GetProcAddress(
        hK32,
        "SetConsoleScreenBufferInfoEx"
    );
    
    return;
};

/* SEC_LIB */
/* * * * * * * * * * * * * * * *
        SECTION CONTENTS
Uncategorized . . . . . SECSUB_LIB@UNCAT
Input Functions . . . . SECSUB_LIB@INPUT
 * * * * * * * * * * * * * * * */

/* SECSUB_LIB_UNCAT SECSUB_LIB@UNCAT */
static int lwcl_NewConsole(lua_State *L) { /* TODO: Come back and finish this. */
    
    /* Get us our new console. */
    FreeConsole();
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    
    /* Now restore things to their proper place. */
    lua_rawgeti(L, UPV_RootTable, RE_OriginalSTDIN);
    lua_rawgeti(L, UPV_RootTable, RE_OriginalSTDOUT);
    lua_rawgeti(L, UPV_RootTable, RE_OriginalSTDERR);
    SetStdHandle(STD_INPUT_HANDLE,  (HANDLE)lua_touserdata(L, -3));
    SetStdHandle(STD_OUTPUT_HANDLE, (HANDLE)lua_touserdata(L, -2));
    SetStdHandle(STD_ERROR_HANDLE,  (HANDLE)lua_touserdata(L, -1));
    lua_pop(L, 3);
    
    lua_pushlightuserdata(L, CreateFileW(
        L"CONIN$",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL));
    FlushConsoleInputBuffer((HANDLE)lua_touserdata(L, -1));
    lua_rawseti(L, UPV_RootTable, RE_CurrentInputBuf);
    
    lwcu_PushScreenBufObj(L, CreateFileW(
        L"CONOUT$",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL));
    SetConsoleActiveScreenBuffer(((ScreenBufObj*)lua_touserdata(L, -1))->Handle);
    
    {CONSOLE_SCREEN_BUFFER_INFO ThisInfo;
     
     GetConsoleScreenBufferInfo(
        ((ScreenBufObj*)lua_touserdata(L, -1))->Handle,
        &ThisInfo);
     lwcu_PushAttrib(L, ThisInfo.wAttributes);
     lua_rawseti(L, UPV_RootTable, RE_DefaultAttrib);
    };
    
    if (ColorFuncsAreAvailable) {
        CONSOLE_SCREEN_BUFFER_INFOEX ThisInfo;
        ThisInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
        
        lua_rawgeti(L, UPV_RootTable, RE_ColorTblLookup);
        lua_pushvalue(L, -2); /* The screenbuffer. */
        GetConsoleScreenBufferInfoEx(
            ((ScreenBufObj*)lua_touserdata(L, -1))->Handle,
            &ThisInfo);
        lwcu_PushColorArray(L, ThisInfo.ColorTable);
        lua_pushvalue(L, -1);
        lua_rawseti(L, UPV_RootTable, RE_DefaultColorTbl);
        
        /* At the moment, the top of the stack contains the color table, below that is the screenbuffer, below that is the ColorTblLookup.*/
        lua_settable(L, -3);
        lua_pop(L, 1); /* Now the stack is as it was before this optional section was entered into. */
    };
        
    lua_rawseti(L, UPV_RootTable, RE_DefaultScreenBuf);
    
    /* Well, I don't really have a good way to detect errors at the moment (not that it'd be hard to think of one), so I'll just
    blithely return true here for no good reason. */
    lua_pushboolean(L, TRUE);

    return 1;
};

static int lwcl_DefaultBuffer(lua_State *L) {
    int numargs = lua_gettop(L);
    
    if        (numargs  > 1) {
        return luaL_error(L, "Invalid number of arguments to function 'DefaultBuffer', please pass exactly 0 or 1 arguments!");
    } else if (numargs == 1) {
        if (!lwcu_IsAScreenBuffer(L, 1)) {
            return luaL_error(L, NotAScreenBufferErrString, 1, "DefaultBuffer");
        };
        lua_rawseti(L, UPV_RootTable, RE_DefaultScreenBuf);
        lua_pushboolean(L, TRUE);
        
        return 1;
    } else { /* if (numargs == 0) */
        lua_rawgeti(L, UPV_RootTable, RE_DefaultScreenBuf);
        
        return 1;
    };
};

static int lwcl_GetNewBuffer(lua_State *L) {
    HANDLE ThisHandle;

    if (lua_gettop(L) != 0) {
        return luaL_error(L, NumArgsErrString, "GetNewBuffer", 0);
    };

    ThisHandle = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL);
    
    if (ThisHandle == INVALID_HANDLE_VALUE) {
        lua_pushnil(L);
        
        return 1;
    };
    
    lwcu_PushScreenBufObj(L, ThisHandle);
    if (ColorFuncsAreAvailable) {
        CONSOLE_SCREEN_BUFFER_INFOEX ThisInfo;
        
        int orig_idx = lua_gettop(L);
        ThisInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
        
        lua_rawgeti(L, UPV_RootTable, RE_DefaultColorTbl);
        
        lua_rawgeti(L, UPV_RootTable, RE_ColorTblLookup);
        lua_pushvalue(L, -3); /* The buffer. */
        lua_pushvalue(L, -3); /* The default color table. */
        lua_settable(L, -3);
        
        #if 0
        
        lua_pop(L, 1); /* Pop the lookup table. */
        
        /* The default color table is on top. */
        GetConsoleScreenBufferInfoEx(ThisHandle, &ThisInfo);
        ThisInfo.srWindow.Right += 1;
        ThisInfo.srWindow.Bottom += 1;
        
        lwcu_ToColorArray(L, -1, ThisInfo.ColorTable);
        lua_pop(L, 1);
        
        SetConsoleScreenBufferInfoEx(ThisHandle, &ThisInfo);
        #endif
        
        lua_settop(L, orig_idx);
    };

    return 1;
};

static int lwcl_SetTitle(lua_State *L) {
    /* Designer's Note:
        It was considered making this function respect __tostring metamethods.
    However, it was determined that making other functions similarly respect
    __tostring in places where strings would be expected, would be far too
    difficult to implement properly, if even possible at all.
        Ultimately, the decision was that sacrificing some ease-of-use here,
    for an admittedly-uncommon situation, was worth keeping useage consistent
    throughout the implementation. Principle of Least Surprise. */
    LPCWSTR ThisTitleString;
    
    if (lua_gettop(L) != 1) {
        return luaL_error(L, NumArgsErrString, "SetTitle", 1);
    } else if (!(lua_isstring(L, 1))) {
        return luaL_error(L, NotAStringOrConvertibleErrString, 1, "SetTitle");
    };
    
    lua_pushliteral(L, "\x00");
    lua_concat(L, 2); /* Have to ensure it's nul-terminated. */
    ThisTitleString = luaW_tostring(L, 1);
    
    lua_pushboolean(L, SetConsoleTitleW(ThisTitleString));

    return 1;
};

/* IMPLEMENTATION-SPECIFIC FUNCTION */
static int lwcl_LowLevelMode(lua_State *L) {
    if (lua_gettop(L) != 0) {
        return luaL_error(L, NumArgsErrString, "LowLevelMode", 0);
    };
    
    lua_rawgeti(L, UPV_RootTable, RE_CurrentInputBuf);
    
    lua_pushboolean(L, SetConsoleMode(
        (HANDLE)lua_touserdata(L, -1),
        ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT)
        );
    
    return 1;
};

/* IMPLEMENTATION-SPECIFIC FUNCTION */
static int lwcl_MakeCoord(lua_State *L) {
    COORD ThisCord;
    
    if (lua_gettop(L) == 1) {
        if (lwcu_IsACoord(L, 1)) {
        
            lua_pushinteger(L, 1);
            lua_gettable(L, -2);
            lua_pushinteger(L, 2);
            lua_gettable(L, -3);
            
            ThisCord.X = lua_tointeger(L, -2);
            ThisCord.Y = lua_tointeger(L, -1);
        } else {
            return luaL_error(L, "Invalid argument #1 to function 'MakeCoord': not a coord! (MakeCoord takes a coord or two numbers)");
        };
    } else if (lua_gettop(L) == 2) {
        if (lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
        
            ThisCord.X = lua_tointeger(L, 1);
            ThisCord.Y = lua_tointeger(L, 2);
        } else {
            return luaL_error(L, "Invalid arguments #1 and/or #2 to function 'MakeCoord': not both numbers! (MakeCoord takes a coord or two numbers)");
        };
    } else {
        return luaL_error(L, NumArgsRangeErrString, "MakeCoord", 1, 2);
    };
    
    lwcu_PushCoord(L, ThisCord, 0);

    return 1;
};

static int lwcl_SetDefaultColorTable(lua_State *L) {
    if (lua_gettop(L) != 1) {
        return luaL_error(L, NumArgsErrString, "SetDefaultColorTable", 1);
    };
    
    if (!lwcu_IsAColorArray(L, 1)) {
        return luaL_error(L, NotAColorArrayErrString, 1, "SetDefaultColorTable");
    };
    
    if (!(ColorFuncsAreAvailable)) {
        lua_pushnil(L);
        return 1;
    };
    
    lua_rawseti(L, UPV_RootTable, RE_DefaultColorTbl);
    
    lua_pushboolean(L, TRUE);

    return 1;
};

static int lwcl_GetDefaultAttrib(lua_State *L) {
    if (lua_gettop(L) != 0) {
        return luaL_error(L, NumArgsErrString, "GetDefaultAttrib", 0);
    };
    
    lua_rawgeti(L, UPV_RootTable, RE_DefaultAttrib);
    
    return 1;
};

/* SECSUB_LIB_INPUT SECSUB_LIB@INPUT */
static int lwcl_ClearInput(lua_State *L) {
    
    if (lua_gettop(L) != 0) {
        return luaL_error(L, NumArgsErrString, "ClearInput", 0);
    };
    
    lua_rawgeti(L, UPV_RootTable, RE_CurrentInputBuf);
    lua_pushboolean(L, FlushConsoleInputBuffer((HANDLE)lua_touserdata(L, -1)));

    return 1;
};

static int lwcl_IsInputAvailable(lua_State *L) {
    INPUT_RECORD ThisInputRecord;
    DWORD        NumberOfRecords;
    HANDLE       ThisHandle;
    
    lua_rawgeti(L, UPV_RootTable, RE_CurrentInputBuf);
    ThisHandle = lua_touserdata(L, -1);
    lua_pop(L, 1);
    
    if (lua_gettop(L) != 0) {
        return luaL_error(L, NumArgsErrString, "IsInputAvailable", 0);
    };
    
    /* The point here is to repeatedly Peek input records until we run out, or we discover
    one which is a KEYBOARD_RECORD or MOUSE_RECORD. We will also discard any input records
    which do not match. */
    GetNumberOfConsoleInputEvents(
        ThisHandle,
        &NumberOfRecords);
    
    if (NumberOfRecords == 0) { 
        lua_pushboolean(L, FALSE);
        return 1;
    };
    
    while (TRUE) {
        BOOL Result = PeekConsoleInputW(
            ThisHandle,
            &ThisInputRecord,
            1,
            &NumberOfRecords);
        
        if (!Result) {
            lua_pushnil(L);
            break;
        };
        
        if (NumberOfRecords == 0) {
            lua_pushboolean(L, FALSE);
            break;
        };
        
        if ((ThisInputRecord.EventType ==   KEY_EVENT) ||
            (ThisInputRecord.EventType == MOUSE_EVENT)) {
            lua_pushboolean(L, TRUE);
            break;
        } else {
            /* The results from this will simply be discarded. */
            ReadConsoleInputW(
                ThisHandle,
                &ThisInputRecord,
                1,
                &NumberOfRecords);
        };
    };
    
    return 1;
};

static int lwcl_FetchInput(lua_State *L) {
    INPUT_RECORD ThisInputRecord;
    HANDLE       ThisHandle;
    DWORD        dummy;
    
    lua_rawgeti(L, UPV_RootTable, RE_CurrentInputBuf);
    ThisHandle = (HANDLE)lua_touserdata(L, -1);
    lua_pop(L, 1);
    
    if (lua_gettop(L) != 0) {
        return luaL_error(L, NumArgsErrString, "FetchInput", 0);
    };
    
    while (TRUE) {
        BOOL Result = ReadConsoleInputW(
            ThisHandle,
            &ThisInputRecord,
            1,
            &dummy);
        
        if (!Result) {
            lua_pushnil(L);
            return 1;
        };
        
        if ((ThisInputRecord.EventType ==   KEY_EVENT) ||
            (ThisInputRecord.EventType == MOUSE_EVENT)) {
            break;
        };
    };
    
    switch (ThisInputRecord.EventType) {
        case KEY_EVENT   :
            lua_pushliteral(L, "key");
            lwcu_PushKeyRecord(L, &ThisInputRecord.Event.KeyEvent);
            break;
        case MOUSE_EVENT :
            lua_pushliteral(L, "mouse");
            lwcu_PushMouseRecord(L, &ThisInputRecord.Event.MouseEvent);
            break;
    };

    return 2;
};

/* SEC_UTIL */
/* * * * * * * * * * * * * * * *
        SECTION CONTENTS
Uncategorized . . . . . SECSUB_UTIL@UNCAT
Coord Functions . . . . SECSUB_UTIL@CORD
Attrib Functions. . . . SECSUB_UTIL@ATTR
ScreenBuffer Functions  SECSUB_UTIL@SCRB
Input Record Functions  SECSUB_UTIL@INRC
Color Array Functions . SECSUB_UTIL@COLARR
 * * * * * * * * * * * * * * * */
 
/* SECSUB_UTIL_UNCAT SECSUB_UTIL@UNCAT */
static void lwcu_Register(lua_State *L, lwc_Reg list[]) {
    /* This is essentially an equivalent to luaL_setfuncs() and luaL_register() but better-suited to our
    particular needs.
       It registers all of the functions given in list[] into the table at the top of the stack. list[]
    must end in an entry whose .Type is RT_SENTRY. */
    {int i;
     for (i = 0; list[i].Type != RT_SENTRY; i++) {
        switch (list[i].Type) {
            case RT_CFUNC :
                lwci_PushUpvalues(L);
                lua_pushcclosure(L, list[i].F.CFunc, NumberOfUpvalues);
                lua_setfield(L, -2, list[i].Name);
                break;
            case RT_LFUNC :
                luaL_loadstring(L, list[i].F.LFunc);
                lua_call(L, 0, 1);
                lua_setfield(L, -2, list[i].Name);
                break;
        };
    };};
    
    return;
};

/* This is a separate Lua function used by several other functions here. It takes one argument, any value,
and returns nothing. It exists solely to be lua_pcall'd. All it does is attempt to perform arg1[1] and arg1[2],
in an attempt to trigger "value cannot be indexed" errors. */
static int lwcu_AttemptToIndex(lua_State *L) {
    if (lua_gettop(L) != 1) { /* Should always have specifically 1 argument, if not, we have a coding error. */
        lua_pushnil(L);
        return lua_error(L);
    };
    
    lua_pushinteger(L, 1);
    lua_gettable(L, 1); /* Index val[1] */
    lua_pushinteger(L, 2);
    lua_gettable(L, 1); /* Index val[2] */
    
    return 0;
};

/* SECSUB_UTIL_CORD SECSUB_UTIL@CORD */
static int lwcu_IsACoord(lua_State *L, int idx) {
    /* Designer's Notes:
        The library specification says that a coord's Value[1] and Value[2] must be "integer numbers,
    or convertible to integer numbers as per the usual Lua rules". This implementation chooses to interpret
    this to mean that a non-integer number, passed through the lua_tointeger() and "truncated in some
    non-specified way", counts as "convertible to [an] integer". 
        Therefore, merely being a number, or a string convertible to a number, is sufficient to qualify
    as a valid value of a coord, however it cannot be guaranteed that the actual values will be used.*/
    
    lua_pushcfunction(L, lwcu_AttemptToIndex);
    lua_pushvalue(L, idx);
    if (lua_pcall(L, 1, 0, 0) != 0) {
        return 0;
    };
    
    lua_pushinteger(L, 1);
    lua_gettable(L, idx); /* lua_gettable(), not lua_rawgeti(). LuaWinCon respects metamethods in this. */
    lua_pushinteger(L, 2);
    lua_gettable(L, idx);
    
    if (lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
        lua_pop(L, 2);
        return 1;
    } else {
        lua_pop(L, 2);
        return 0;
    };
};

static int lwcu_IsAPositionCoord(lua_State *L, int idx) {
    /* THIS FUNCTION ASSUMES THAT THE VALUE IS ALREADY KNOWN TO BE A VALID COORD.
    BE SURE TO RUN THE VALUE THROUGH lwcu_IsACoord() FIRST BEFORE PASSING IT THROUGH
    THIS FUNCTION. */
    
    lua_pushinteger(L, 1);
    lua_gettable(L, idx);
    lua_pushinteger(L, 2);
    lua_gettable(L, idx);
    
    if ((lua_tointeger(L, -2) > 0) && (lua_tointeger(L, -1) > 0)) {
        lua_pop(L, 2);
        return 1;
    } else {
        lua_pop(L, 2);
        return 0;
    };
};

static int lwcu_IsASizeCoord(lua_State *L, int idx) {
    /* THIS FUNCTION ASSUMES THAT THE VALUE IS ALREADY KNOWN TO BE A VALID COORD.
    BE SURE TO RUN THE VALUE THROUGH lwcu_IsACoord() FIRST BEFORE PASSING IT THROUGH
    THIS FUNCTION. */
    
    lua_pushinteger(L, 1);
    lua_gettable(L, idx);
    lua_pushinteger(L, 2);
    lua_gettable(L, idx);
    
    if ((lua_tointeger(L, -2) >= 0) && (lua_tointeger(L, -1) >= 0)) {
        lua_pop(L, 2);
        return 1;
    } else {
        lua_pop(L, 2);
        return 0;
    };
};

static void lwcu_PushCoord(lua_State *L, COORD C, signed int Adjustment) {
    /* Pushes a 'coord' onto the stack with the appropriate metatable and everything. Its values
    correspond to the values of 'C', adjusted by the value of 'Adjustment'. */
    
    lua_createtable(L, 2, 0);
    lua_pushinteger(L, C.X + Adjustment);
    lua_rawseti(L, -2, 1);
    lua_pushinteger(L, C.Y + Adjustment);
    lua_rawseti(L, -2, 2);
    
    lua_rawgeti(L, UPV_RootTable, RE_CoordMetaTable);
    lua_setmetatable(L, -2);
    
    return;
};

static COORD lwcu_ToCoord(lua_State *L, int idx, signed int Adjustment) {
    /* THIS FUNCTION ASSUMES THAT THE VALUE AT 'idx' IS ALREADY KNOWN TO BE A VALID COORD.
    BE SURE TO RUN THE VALUE THROUGH lwcu_IsACoord() FIRST BEFORE PASSING IT THROUGH THIS
    FUNCTION.
       Takes the coord at stack index 'idx' and converts it into a COORD, applying the
    value 'Adjustment' to the interior values of the COORD. */
    COORD RetCoord;
    
    lua_pushinteger(L, 1);
    lua_gettable(L, idx);
    RetCoord.X = (SHORT)(lua_tointeger(L, -1) + Adjustment);
    lua_pushinteger(L, 2);
    lua_gettable(L, idx);
    RetCoord.Y = (SHORT)(lua_tointeger(L, -1) + Adjustment);
    
    return RetCoord;
};

/* SECSUB_UTIL_ATTR SECSUB_UTIL@ATTR */
static int lwcu_IsAnAttrib(lua_State *L, int idx) {
    /* Designer's notes: 
        According to specifications, an implementation is absolutely free to define the structure
    of an 'attrib', or even to choose not to define it all and instead provide some opaque values
    to use as attribs.
        Our implementation, as usual, reflects the way the actual Win32 Console works. However, the
    actual way it works is not the same way that most programs usually interact with it, at a low
    level.
        Usually, one builds up a color by using a mixture of bitwise flags called:
            FOREGROUND_BLUE         BACKGROUND_BLUE
            FOREGROUND_GREEN        BACKGROUND_GREEN
            FOREGROUND_RED          BACKGROUND_RED
            FOREGROUND_INTENSITY    BACKGROUND_INTENSITY
            (along with some additional flags used only for Far-East languages)
        And thus, one might expect that we would define an 'attrib' as an array of two strings used
    as enumeration values for predefined colors like "Red", "Bright Red", "Cyan", etc.
        However, in reality, those bitwise flags build up a 4-bit index into a screenbuffer's
    "Color Table", which actually contains 24-bit RGB values. So, we have defined an attrib as being
    any value for which value[1] and value[2] produce integers (or values convertible to integers as
    per usual Lua rules) in the range 1..16. value[1] indicates the foreground color, and value[2]
    indicates the background color.
        As with coords, this function will perform the usual Lua truncation of non-integer numbers. */

    lua_pushcfunction(L, lwcu_AttemptToIndex);
    lua_pushvalue(L, idx);
    if (lua_pcall(L, 1, 0, 0) != 0) {
        return 0;
    };
    
    lua_pushinteger(L, 1);
    lua_gettable(L, idx);
    lua_pushinteger(L, 2);
    lua_gettable(L, idx);
    
    if (!(lua_isnumber(L, -2) && lua_isnumber(L, -1))) {
        lua_pop(L, 2);
        return 0;
    } else {
        lua_Integer Candidate1 = lua_tointeger(L, -2);
        lua_Integer Candidate2 = lua_tointeger(L, -1);
        if (!(((Candidate1 >= 1) && (Candidate1 <= 16)) && ((Candidate2 >= 1) && Candidate2 <= 16))) {
            lua_pop(L, 2);
            return 0;
        } else {
            lua_pop(L, 2);
            return 1;
        };
    };
};

static WORD lwcu_ToAttrib(lua_State *L, int idx) {
    /* This function assumes the value has already been checked for validity! Do not run this
    function without first running lwcu_IsAnAttrib() on the value! */
    
    WORD ThisAttrib = 0;
    
    lua_pushinteger(L, 1);
    lua_gettable(L, idx);
    ThisAttrib = (((WORD)lua_tointeger(L, -1) - 1) & 0x0F);
    
    lua_pushinteger(L, 2);
    lua_gettable(L, idx);
    ThisAttrib |= ((((WORD)lua_tointeger(L, -1) - 1) & 0x0F) << 4);
    
    lua_pop(L, 2);
    
    return ThisAttrib;
};

static void lwcu_PushAttrib(lua_State *L, WORD attr) {
    lua_createtable(L, 2, 0);
    
    lua_pushinteger(L, (attr & 0x0F)+1);
    lua_rawseti(L, -2, 1);
    
    lua_pushinteger(L, ((attr & 0xF0) >> 4)+1);
    lua_rawseti(L, -2, 2);

    return;
};

/* SECSUB_UTIL_SCRB SECSUB_UTIL@SCRB */
static int lwcu_IsAScreenBuffer(lua_State *L, int idx) {
    if (!(lua_isuserdata(L, idx))) {
        return 0;
    } else if (!(lua_getmetatable(L, idx))) {/* This pushes the metatable onto the stack if it succeeds. */
        return 0;
    } else {
        lua_rawgeti(L, UPV_RootTable, RE_ScreenBufMetaTable);
        if (!lua_equal(L, -2, -1)) {
            lua_pop(L, 2);
            return 0;
        } else {
            lua_pop(L, 2);
            return 1;
        };
    };
};

static void lwcu_PushScreenBufObjWithinLuaOpen(lua_State *L, HANDLE ThisHandle) {
    /* This is just like the normal version of lwcu_PushScreenBufObj except it's designed to work
    within the luaopen_LuaWinCon function, where we cannot access the shared RootTable as an upvalue. */
    ScreenBufObj* ThisBufObj = (ScreenBufObj*)lua_newuserdata(L, sizeof(ScreenBufObj));
    ThisBufObj->Handle = ThisHandle;
    lua_rawgeti(L, RootTableStackIdx, RE_ScreenBufMetaTable);
    lua_setmetatable(L, -2);

    return;
};

static void lwcu_PushScreenBufObj(lua_State *L, HANDLE ThisHandle) {
    ScreenBufObj* ThisBufObj = (ScreenBufObj*)lua_newuserdata(L, sizeof(ScreenBufObj));
    ThisBufObj->Handle = ThisHandle;
    lua_rawgeti(L, UPV_RootTable, RE_ScreenBufMetaTable);
    lua_setmetatable(L, -2);
    
    return;
};

/* SECSUB_UTIL_INRC SECSUB_UTIL@INRC */
#define PushControlKeyState(L, Keys) \
            lua_pushboolean(L, Keys & CAPSLOCK_ON); \
            lua_setfield(L, -2, "CapsLockIsOn"); \
            lua_pushboolean(L, Keys & NUMLOCK_ON); \
            lua_setfield(L, -2, "NumLockIsOn"); \
            lua_pushboolean(L, Keys & SCROLLLOCK_ON); \
            lua_setfield(L, -2, "ScrollLockIsOn"); \
            lua_pushboolean(L, Keys & SHIFT_PRESSED); \
            lua_setfield(L, -2, "ShiftIsDown"); \
            lua_pushboolean(L, Keys & LEFT_ALT_PRESSED); \
            lua_setfield(L, -2, "LeftAltIsDown"); \
            lua_pushboolean(L, Keys & LEFT_CTRL_PRESSED); \
            lua_setfield(L, -2, "LeftCtrlIsDown"); \
            lua_pushboolean(L, Keys & RIGHT_ALT_PRESSED); \
            lua_pushvalue(L, -1); \
            lua_setfield(L, -3, "RightAltIsDown"); \
            lua_setfield(L, -2, "AltGrIsDown"); \
            lua_pushboolean(L, Keys & RIGHT_CTRL_PRESSED); \
            lua_setfield(L, -2, "RightCtrlIsDown")

static void lwcu_PushKeyRecord(lua_State *L, KEY_EVENT_RECORD *K) {
    lua_newtable(L);
    
    lua_pushboolean(L, K->bKeyDown);
    lua_setfield(L, -2, "IsDown");
    
    lua_pushinteger(L, (lua_Integer)K->wRepeatCount);
    lua_setfield(L, -2, "RepeatCount");
    
    lua_pushinteger(L, (lua_Integer)K->wVirtualKeyCode);
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, "KeyCode");
    lua_rawget(L, UPV_KeyCodeTranslationTable);
    lua_setfield(L, -2, "Key");
    
    lua_pushinteger(L, (lua_Integer)K->wVirtualScanCode);
    lua_setfield(L, -2, "ScanCode");
    
    luaW_pushlstring(L, &K->uChar.UnicodeChar, (K->uChar.UnicodeChar == 0x0000) ? 0 : 1);
    lua_setfield(L, -2, "Character");
    
    {DWORD ControlKeys = K->dwControlKeyState;
      
      lua_pushboolean(L, ControlKeys & ENHANCED_KEY);
      lua_setfield(L, -2, "IsEnhancedKey");
      
      PushControlKeyState(L, ControlKeys);
    };

    return;
};

static void lwcu_PushMouseRecord(lua_State *L, MOUSE_EVENT_RECORD *M) {
    lua_newtable(L);
    
    /* Push mouse position. */
    lwcu_PushCoord(L, M->dwMousePosition, 1);
    lua_setfield(L, -2, "Position");
    
    /* Push mouse button table. */
    /* We used to make this in a loop, but we might as well do it manually now. */
    lua_createtable(L, 5, 2);
    {DWORD ButtonState = M->dwButtonState;
    
      lua_pushboolean(L, ButtonState & FROM_LEFT_1ST_BUTTON_PRESSED);
      lua_pushvalue(L, -1);
      lua_rawseti(L, -3, 1);
      lua_setfield(L, -2, "left");
      
      lua_pushboolean(L, ButtonState & RIGHTMOST_BUTTON_PRESSED);
      lua_pushvalue(L, -1);
      lua_rawseti(L, -3, 2);
      lua_setfield(L, -2, "right");
      
      lua_pushboolean(L, ButtonState & FROM_LEFT_2ND_BUTTON_PRESSED);
      lua_rawseti(L, -2, 3);
      
      lua_pushboolean(L, ButtonState & FROM_LEFT_3RD_BUTTON_PRESSED);
      lua_rawseti(L, -2, 4);
      
      lua_pushboolean(L, ButtonState & FROM_LEFT_4TH_BUTTON_PRESSED);
      lua_rawseti(L, -2, 5);
    };
    lua_setfield(L, -2, "Buttons");
    
    /* Push "Action", and "Direction", if necessary. */
    switch (M->dwEventFlags) {
        case 0x0000 : /* A button was pressed or released. */
            lua_pushliteral(L, "buttoned");
            break;
        case DOUBLE_CLICK :
            lua_pushliteral(L, "doubleclicked");
            break;
        case MOUSE_MOVED :
            lua_pushliteral(L, "moved");
            break;
        case MOUSE_WHEELED :
            lua_pushstring(L, (M->dwButtonState >> 16) ? "awayfrom" : "towards");
            lua_setfield(L, -2, "Direction");
            lua_pushliteral(L, "wheeled");
            break;
        case MOUSE_HWHEELED :
            lua_pushstring(L, (M->dwButtonState >> 16) ? "right" : "left");
            lua_setfield(L, -2, "Direction");
            lua_pushliteral(L, "hwheeled");
            break;
        default :
            luaL_error(L, "Impossible condition encountered when pushing a mouse input record table: unknown event flag encountered!");
    };
    lua_setfield(L, -2, "Action");
    
    {DWORD ControlKeys = M->dwControlKeyState;
    
      PushControlKeyState(L, ControlKeys);
    };
    
    return;
};
#undef PushControlKeyState

/* SECSUB_UTIL_COLARR SECSUB_UTIL@COLARR */
static void lwcu_PushColorArray(lua_State *L, COLORREF color_tbl[]) {
    lua_createtable(L, 16, 0);
    
    {int aidx;
     for (aidx = 1; aidx <= 16; aidx++) {
        COLORREF ThisCol = color_tbl[aidx-1];
        
        lua_createtable(L, 0, 3);
        
        lua_pushnumber(L, ((lua_Number)GetRValue(ThisCol)) / 255.0);
        lua_setfield(L, -2, "r");
        
        lua_pushnumber(L, ((lua_Number)GetGValue(ThisCol)) / 255.0);
        lua_setfield(L, -2, "g");
        
        lua_pushnumber(L, ((lua_Number)GetBValue(ThisCol)) / 255.0);
        lua_setfield(L, -2, "b");
        
        lua_rawseti(L, -2, aidx);
    };};

    return;
};

static const char* color_indices[] = {"r","g","b"};

static void lwcu_ToColorArray(lua_State *L, int idx, COLORREF color_tbl[]) {
    /* This function must be called after lwcu_IsAColorArray(), it will *not* verify its argument. */
    int orig_idx = lua_gettop(L);
    
    lua_pushvalue(L, idx);
    {int aidx;
     for (aidx = 1; aidx <= 16; aidx++) {
        BYTE Channels[3];
        
        lua_pushinteger(L, aidx);
        lua_gettable(L, -2);
        
        {int cidx;
         for (cidx = 0; cidx <= 2; cidx++) {
            lua_getfield(L, -1, color_indices[cidx]);
            Channels[cidx] = (BYTE)(lua_tonumber(L, -1) * 255.0);
            lua_pop(L, 1);
        };};
        
        color_tbl[aidx-1] = RGB((Channels[0]), (Channels[1]), (Channels[2]));
        lua_pop(L, 1);
    };};
    
    lua_settop(L, orig_idx);

    return;
};

static int lwcu_IsAColorArray(lua_State *L, int sidx) {
    int orig_idx = lua_gettop(L);
    
    lua_pushvalue(L, sidx); /* Put a copy of it onto the top of our stack. */
    lua_pushcfunction(L, lwcu_AttemptToIndex);
    lua_pushvalue(L, -2);
    if (lua_pcall(L, 1, 0, 0) != 0) {
        lua_print(L, "Base Array failed AttemptToIndex");
        lua_settop(L, orig_idx);
        return 0;
    };
    
    {int aidx;
     for (aidx = 1; aidx <= 16; aidx++) {
        lua_pushinteger(L, aidx);
        lua_gettable(L, -2);
        
        lua_pushcfunction(L, lwcu_AttemptToIndex);
        lua_pushvalue(L, -2);
        if (lua_pcall(L, 1, 0, 0) != 0) {
            lua_print(L, "Color failed AttemptToIndex");
            lua_settop(L, orig_idx);
            return 0;
        };
        
        {int cidx;
         for (cidx = 0; cidx <= 2; cidx++) {
            lua_getfield(L, -1, color_indices[cidx]);
            if (!lua_isnumber(L, -1)) {
                lua_print(L, "Channel is not a number");
                lua_settop(L, orig_idx);
                return 0;
            } else {
                lua_Number Channel;
                Channel = lua_tonumber(L, -1);
                if ((Channel < 0.0) || (Channel > 1.0)) {
                    lua_print(L, "Channel outside valid range.");
                    lua_settop(L, orig_idx);
                    return 0;
                };
            };
            lua_pop(L, 1);
        };};
        lua_pop(L, 1);
    };};
    
    lua_settop(L, orig_idx);

    return 1;
};

/* SEC_METHS */
static int lwcsbm_Close(lua_State *L) {
    if (lua_gettop(L) != 1) {
        return luaL_error(L, NumArgsErrString, "ScreenBuf:Close", 0);
    };
    if (!(lwcu_IsAScreenBuffer(L, 1))) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:Close");
    };
    
    CloseHandle(((ScreenBufObj*)lua_touserdata(L, 1))->Handle);
    
    return 0;
};

static int lwcsbm_MakeActive(lua_State *L) {
    HANDLE ThisHandle;

    if (lua_gettop(L) != 1) {
        return luaL_error(L, NumArgsErrString, "ScreenBuf:MakeActive", 0);
    };
    if (!(lwcu_IsAScreenBuffer(L, 1))) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:MakeActive");
    };
    
    ThisHandle = ((ScreenBufObj*)lua_touserdata(L, 1))->Handle;
    
    if (ColorFuncsAreAvailable) {
        CONSOLE_SCREEN_BUFFER_INFOEX ThisInfo;
        ThisInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    
        lua_rawgeti(L, UPV_RootTable, RE_ColorTblLookup);
        lua_pushvalue(L, -2);
        lua_gettable(L, -2);
        
        GetConsoleScreenBufferInfoEx(ThisHandle, &ThisInfo);
        ThisInfo.srWindow.Right += 1;
        ThisInfo.srWindow.Bottom += 1;
        
        lwcu_ToColorArray(L, -1, ThisInfo.ColorTable);
        SetConsoleScreenBufferInfoEx(ThisHandle, &ThisInfo);
    }; /* No need to clean up the stack here. */
    
    lua_pushboolean(L, SetConsoleActiveScreenBuffer(ThisHandle));
    
    return 1;
};

static int lwcsbm_GetLargestWindowSize(lua_State *L) {
    if (lua_gettop(L) != 1) {
        return luaL_error(L, NumArgsErrString, "ScreenBuf:GetLargestWindowSize", 0);
    };
    if (!(lwcu_IsAScreenBuffer(L, 1))) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:GetLargestWindowSize");
    };
    
    lwcu_PushCoord(L, GetLargestConsoleWindowSize(((ScreenBufObj*)lua_touserdata(L, 1))->Handle), 0);

    return 1;
};

static int lwcsbm_SetCursorVisible(lua_State *L) {
    CONSOLE_CURSOR_INFO ThisInfo;
    HANDLE              ThisHandle;
    
    if (lua_gettop(L) != 2) {
        return luaL_error(L, NumArgsErrString, "ScreenBuf:SetCursorVisible", 1);
    };
    if (!(lwcu_IsAScreenBuffer(L, 1))) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:SetCursorVisible");
    } else {
        ThisHandle = ((ScreenBufObj*)lua_touserdata(L, 1))->Handle;
    };
    if (!(lua_isnil(L, 2) || lua_isboolean(L, 2))) {
        return luaL_error(L, "Invalid argument #1 to function 'ScreenBuf:SetCursorVisible': not a boolean or explicit nil!");
    };
    
    if (!GetConsoleCursorInfo(ThisHandle, &ThisInfo)) {
        lua_pushboolean(L, FALSE);
        return 1;
    };
    
    ThisInfo.bVisible = lua_toboolean(L, 2);
    
    lua_pushboolean(L, SetConsoleCursorInfo(ThisHandle, &ThisInfo));
    
    return 1;
};

static int lwcsbm_SetCursorPosition(lua_State *L) {
    if (lua_gettop(L) != 2) {
        return luaL_error(L, NumArgsErrString, "ScreenBuf:SetCursorPosition", 1);
    };
    if (!(lwcu_IsAScreenBuffer(L, 1))) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:SetCursorPosition");
    };
    if (!lwcu_IsACoord(L, 2)) {
        return luaL_error(L, NotACoordErrString, 1, "ScreenBuf:SetCursorPosition");
    } else if (!lwcu_IsAPositionCoord(L, 2)) {
        return luaL_error(L, NotAPositionCoordErrString, 1, "ScreenBuf:SetCursorPosition");
    };
    
    lua_pushboolean(L, SetConsoleCursorPosition(
        ((ScreenBufObj*)lua_touserdata(L, 1))->Handle,
        lwcu_ToCoord(L, 2, -1)
        )); 
    
    return 1;
};

static int lwcsbm_SetCursorSize(lua_State *L) {
    CONSOLE_CURSOR_INFO ThisInfo;
    HANDLE              ThisHandle;
    lua_Number          ThisNumber;
    
    if (lua_gettop(L) != 2) {
        return luaL_error(L, NumArgsErrString, "ScreenBuf:SetCursorSize", 1);
    };
    if (!(lwcu_IsAScreenBuffer(L, 1))) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:SetCursorSize");
    } else {
        ThisHandle = ((ScreenBufObj*)lua_touserdata(L, 1))->Handle;
    };
    if (!lua_isnumber(L, 2)) {
        return luaL_error(L, NotANumberOrConvertibleErrString, 1, "ScreenBuf:SetCursorSize");
    } else {
        ThisNumber = lua_tonumber(L, 2);
    };
    if ((ThisNumber < 0.0) || (ThisNumber > 1.0)) {
        return luaL_error(L, "Invalid argument #1 to function 'ScreenBuf:SetCursorSize': number outside legal range!");
    };
    
    if (!GetConsoleCursorInfo(ThisHandle, &ThisInfo)) {
        lua_pushboolean(L, FALSE);
        return 1;
    };
    
    /* SetConsoleCursorInfo expects dwSize to be in the range 1..100 */
    ThisInfo.dwSize = (DWORD)((ThisNumber * 99) + 1);
    
    lua_pushboolean(L, SetConsoleCursorInfo(ThisHandle, &ThisInfo));
    
    return 1;
};

static int lwcsbm_SetScreenBufferSize(lua_State *L) {
    if (lua_gettop(L) != 2) {
        return luaL_error(L, NumArgsErrString, "ScreenBuf:SetScreenBufferSize", 1);
    };
    if (!(lwcu_IsAScreenBuffer(L, 1))) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:SetScreenBufferSize");
    };
    if (!lwcu_IsACoord(L, 2)) {
        return luaL_error(L, NotACoordErrString, 1, "ScreenBuf:SetScreenBufferSize");
    } else if (!lwcu_IsASizeCoord(L, 2)) {
        return luaL_error(L, NotASizeCoordErrString, 1, "ScreenBuf:SetScreenBufferSize");
    };
    
    lua_pushboolean(L, SetConsoleScreenBufferSize(
        ((ScreenBufObj*)lua_touserdata(L, 1))->Handle,
        lwcu_ToCoord(L, 2, 0))
        );
    
    return 1;
};

static int lwcsbm_SetWindowPositionAndSize(lua_State *L) {
    SMALL_RECT ThisRect;
    COORD      ThisPos;
    COORD      ThisSize;

    if (lua_gettop(L) != 3) {
        return luaL_error(L, NumArgsErrString, "ScreenBuf:SetWindowPositionAndSize", 2);
    };
    if (!(lwcu_IsAScreenBuffer(L, 1))) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:SetWindowPositionAndSize");
    };
    if (!lwcu_IsACoord(L, 2)) {
        return luaL_error(L, NotACoordErrString, 1, "ScreenBuf:SetWindowPositionAndSize");
    } else if (!lwcu_IsAPositionCoord(L, 2)) {
        return luaL_error(L, NotAPositionCoordErrString, 1, "ScreenBuf:SetWindowPositionAndSize");
    };
    if (!lwcu_IsACoord(L, 3)) {
        return luaL_error(L, NotACoordErrString, 2, "ScreenBuf:SetWindowPositionAndSize");
    } else if (!lwcu_IsASizeCoord(L, 3)) {
        return luaL_error(L, NotASizeCoordErrString, 2, "ScreenBuf:SetWindowPositionAndSize");
    };
    
    ThisPos  = lwcu_ToCoord(L, 2, -1);
    ThisSize = lwcu_ToCoord(L, 3, 0);
    
    ThisRect.Left   = ThisPos.X;
    ThisRect.Top    = ThisPos.Y;
    ThisRect.Right  = ThisPos.X + ThisSize.X - 1;
    ThisRect.Bottom = ThisPos.Y + ThisSize.Y - 1;
    
    lua_pushboolean(L, SetConsoleWindowInfo(
        ((ScreenBufObj*)lua_touserdata(L, 1))->Handle,
        TRUE,
        &ThisRect)
        );
    
    return 1;
};

static int lwcsbm_FillRegion(lua_State *L) {
    HANDLE ThisHandle;
    COORD  ThisPos;
    COORD  ThisSize;
    WCHAR  ThisWChar;
    BOOL   UsingChar = FALSE;
    WORD   ThisAttrib;
    BOOL   UsingAttrib = FALSE;
    
    int NumArgs = lua_gettop(L);
    if ((NumArgs > 5) || (NumArgs < 3)) {
        return luaL_error(L, NumArgsRangeErrString, "ScreenBuf:FillRegion", 2, 4);
    };
    
    if (!lwcu_IsAScreenBuffer(L, 1)) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:FillRegion");
    } else {
        ThisHandle = ((ScreenBufObj*)lua_touserdata(L, 1))->Handle;
    };
    
    /* Now to validate arguments. */
    if        (!lwcu_IsACoord(L, 2)) {
        return luaL_error(L, NotACoordErrString, 1, "ScreenBuf:FillRegion");
    } else if (!lwcu_IsAPositionCoord(L, 2)) {
        return luaL_error(L, NotAPositionCoordErrString, 1, "ScreenBuf:FillRegion");
    } else {
        ThisPos = lwcu_ToCoord(L, 2, -1);
    };
    
    if        (!lwcu_IsACoord(L, 3)) {
        return luaL_error(L, NotACoordErrString, 2, "ScreenBuf:FillRegion");
    } else if (!lwcu_IsASizeCoord(L, 3)) {
        return luaL_error(L, NotASizeCoordErrString, 2, "ScreenBuf:FillRegion");
    } else {
        ThisSize = lwcu_ToCoord(L, 3, 0);
    };
    
    if (NumArgs >= 4) { /* We have a character given. */
        if (!lua_isnoneornil(L, 4)) {
            if (lua_isstring(L, 4)) {
                UsingChar = (lua_rawlen(L, 4) > 0) ? TRUE : FALSE;
            } else {
                return luaL_error(L, NotAStringOrConvertibleErrString, 3, "ScreenBuf:FillRegion");
            };
        };
    };
    
    if (NumArgs == 5) { /* We have an attribute given. */
        if (!lua_isnoneornil(L, 5)) {
            if (lwcu_IsAnAttrib(L, 5)) {
                UsingAttrib = TRUE;
                ThisAttrib = lwcu_ToAttrib(L, 5);
            } else {
                return luaL_error(L, NotAnAttribErrString, 4, "ScreenBuf:FillRegion");
            };
        };
    };
    
    if ((ThisSize.X == 0) || (ThisSize.Y == 0)) {
        lua_pushboolean(L, TRUE);
        return 1;
    };
    
    if ((!UsingChar) && (!UsingAttrib)) {
        lua_pushboolean(L, TRUE);
        return 1;
    };
    
    ThisWChar = luaW_tostring(L, 4)[0];
    
    {COORD WorkingPos;
     BOOL  Success = TRUE;
     DWORD Dummy; /* Sometimes there are problems if we don't do this. */
     for (WorkingPos = ThisPos; WorkingPos.Y < (ThisPos.Y + ThisSize.Y); WorkingPos.Y += 1) {
        if (UsingChar) {
            Success |= (TRUE ^ FillConsoleOutputCharacterW( /* TODO: This method of implementing sticky failure is awkward. */
                                ThisHandle,
                                ThisWChar,
                                ThisSize.X,
                                WorkingPos,
                                &Dummy));
        };
        if (UsingAttrib) {
            Success |= (TRUE ^ FillConsoleOutputAttribute(
                                ThisHandle,
                                ThisAttrib,
                                ThisSize.X,
                                WorkingPos,
                                &Dummy));
        };
    };
    
    lua_pushboolean(L, Success);
    return 1;
    };
};

static int lwcsbm_WriteRegion(lua_State *L) {
    HANDLE       ThisHandle;
    unsigned int NumArgs;
    COORD        ThisPos;
    COORD        ThisSize;
    BOOL         Success = TRUE; /* Like with other multi-function-call functions, failure is "sticky" here. */
    
    COORD        ThisRealSize;
    
    WORD         CurrentAttrib;
    BOOL         UsingAttrib = FALSE;
    
    NumArgs = lua_gettop(L);
    if (NumArgs < 3) {
        return luaL_error(L, "Invalid number of arguments to function 'ScreenBuf:WriteRegion', please pass at least 2 arguments!");
    };
    
    if (!lwcu_IsAScreenBuffer(L, 1)) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:WriteRegion");
    } else {
        ThisHandle = ((ScreenBufObj*)lua_touserdata(L, 1))->Handle;
    };
    
    if (!lwcu_IsACoord(L, 2)) {
        return luaL_error(L, NotACoordErrString, 1, "ScreenBuf:WriteRegion");
    } else if (!lwcu_IsAPositionCoord(L, 2)) {
        return luaL_error(L, NotAPositionCoordErrString, 1, "ScreenBuf:WriteRegion");
    } else {
        ThisPos = lwcu_ToCoord(L, 2, -1);
    };
    
    if (!lwcu_IsACoord(L, 3)) {
        return luaL_error(L, NotACoordErrString, 2, "ScreenBuf:WriteRegion");
    } else if (!lwcu_IsASizeCoord(L, 3)) {
        return luaL_error(L, NotASizeCoordErrString, 2, "ScreenBuf:WriteRegion");
    } else {
        ThisSize = lwcu_ToCoord(L, 3, 0);
    };
    
    /* Now we first loop through all arguments in an attempt to verify them. */
    if (NumArgs > 3) {
        unsigned int idx;
        for (idx = 4; idx <= NumArgs; idx++) {
            if        (lua_isnil(L, idx)) {
                continue;
            } else if (lwcu_IsAnAttrib(L, idx)) {
                /* It is explicitly required that we check for being an attrib, before checking for being a number or string. */
                continue;
            } else if (lua_type(L, idx) == LUA_TNUMBER) {
                if (lua_tointeger(L, idx) < 0) {
                    return luaL_error(L, "Invalid argument #%d to function 'ScreenBuf:WriteRegion': number outside legal range!",
                        idx - 1);
                } else {
                    continue;
                };
            } else if (lua_type(L, idx) == LUA_TSTRING) {
                continue;
            } else { /* Not one of our acknowledged valid types. */
                return luaL_error(L, "Invalid argument #%d to function 'ScreenBuf:WriteRegion': not a nil, attrib, number, or string!",
                    idx - 1);
            };
        };
    } else {
        /* If we have nothing in our variadic section, just return true. */
        lua_pushboolean(L, TRUE);
        return 1;
    };
    
    /* If either of the components of our 'size' attrib are 0, return true. */
    if ((ThisSize.X == 0) || (ThisSize.Y == 0)) {
        lua_pushboolean(L, TRUE);
        return 1;
    };
    
    /* And now to effect our arguments. */
    
    /* First off, we have to find out the actual size of the screenbuffer so we know where to clip our arguments to.
    We are performing most of the positioning and clipping of the data ourselves. The last version read in all of the
    data corresponding to where we wanted to write to, advanced through it, overwriting data as necessary, and then re-
    copied this back to the console. This used up more memory transfers than was necessary, so this time, we're taking
    a more complex route that involves a lot of function calls to carefully-calculated positions and lengths, but I'm
    hoping this will be more efficient. */
    {CONSOLE_SCREEN_BUFFER_INFO ThisInfo;
        if (!GetConsoleScreenBufferInfo(ThisHandle, &ThisInfo)) {
            lua_pushboolean(L, FALSE);
            return 1;
        };
        ThisRealSize = ThisInfo.dwSize;
    };
    
    /* Let's see if the origin of our region is even inside the extent of the screenbuffer's region. */
    if ((ThisPos.X >= ThisRealSize.X) || (ThisPos.Y >= ThisRealSize.Y)) {
        lua_pushboolean(L, TRUE);
        return 1;
    };

    /* Let's define some useful values. */
    {unsigned int RegionCursor = 0;
     unsigned int FirstClippedColumn = ((ThisPos.X + ThisSize.X) > ThisRealSize.X) ?
                                            (ThisRealSize.X - ThisPos.X) :
                                            ThisSize.X;
     unsigned int FirstInvalidCursor = ThisSize.X * ThisSize.Y;
     
     unsigned int idx;
     for (idx = 4; ((idx <= NumArgs) && (RegionCursor < FirstInvalidCursor)); idx++) {
        
        /* NIL HANDLER */
        if        (lua_isnil(L, idx)) {
            UsingAttrib = FALSE;
            continue;
        
        /* ATTRIB HANDLER */
        } else if (lwcu_IsAnAttrib(L, idx)) {
            UsingAttrib = TRUE;
            CurrentAttrib = lwcu_ToAttrib(L, idx);
            continue;
        
        /* NUMBER HANDLER */
        } else if (lua_type(L, idx) == LUA_TNUMBER) {
            unsigned int CellsToWrite = lua_tointeger(L, idx);
            
            if (!UsingAttrib) {
                RegionCursor += CellsToWrite;
                continue;
            
            } else {
                while ((CellsToWrite > 0) && (RegionCursor < FirstInvalidCursor)) {
                    COORD LocalWritePos = {
                        (RegionCursor % ThisSize.X),
                        (RegionCursor / ThisSize.X)};
                    COORD AbsoluteWritePos = {
                        (ThisPos.X + LocalWritePos.X),
                        (ThisPos.Y + LocalWritePos.Y)};
                    DWORD WriteLength = (CellsToWrite < (FirstClippedColumn - LocalWritePos.X)) ?
                                            CellsToWrite :
                                            (FirstClippedColumn - LocalWritePos.X);
                    unsigned int AdvanceLength = (CellsToWrite < (ThisSize.X - LocalWritePos.X)) ?
                                                    CellsToWrite :
                                                    (ThisSize.X - LocalWritePos.X);
                    DWORD dummy; /* I've found this is necessary in many cases. */
                    
                    Success |= (TRUE ^ FillConsoleOutputAttribute(
                                            ThisHandle,
                                            CurrentAttrib,
                                            WriteLength,
                                            AbsoluteWritePos,
                                            &dummy));
                    
                    RegionCursor += AdvanceLength;
                    CellsToWrite -= AdvanceLength;
                };
                continue;
            };
        
        /* STRING HANDLER */
        } else if (lua_type(L, idx) == LUA_TSTRING) {
            unsigned int ElementCursor = 0;
            size_t       ElementLength;
            LPCWSTR      ThisStr = luaW_tolstring(L, idx, &ElementLength);
            
            if (ElementLength == 0) {
                lua_pop(L, 1);
                continue;
            
            } else {
                while ((ElementCursor < ElementLength) && (RegionCursor < FirstInvalidCursor)) {
                    COORD LocalWritePos = {
                        (RegionCursor % ThisSize.X),
                        (RegionCursor / ThisSize.X)};
                    COORD AbsoluteWritePos = {
                        (ThisPos.X + LocalWritePos.X),
                        (ThisPos.Y + LocalWritePos.Y)};
                    DWORD WriteLength = ((ElementLength - ElementCursor) < (FirstClippedColumn - LocalWritePos.X)) ?
                                            (ElementLength - ElementCursor) :
                                            (FirstClippedColumn - LocalWritePos.X);
                    unsigned int AdvanceLength = ((ElementLength - ElementCursor) < (ThisSize.X - LocalWritePos.X)) ?
                                                    (ElementLength - ElementCursor) :
                                                    (ThisSize.X - LocalWritePos.X);
                    DWORD dummy;
                    
                    Success |= (TRUE ^ WriteConsoleOutputCharacterW(
                                            ThisHandle,
                                            (ThisStr + ElementCursor),
                                            WriteLength,
                                            AbsoluteWritePos,
                                            &dummy));
                    
                    if (UsingAttrib) {
                        Success |= (TRUE ^ FillConsoleOutputAttribute(
                                                ThisHandle,
                                                CurrentAttrib,
                                                WriteLength,
                                                AbsoluteWritePos,
                                                &dummy));
                    };
                    
                    RegionCursor += AdvanceLength;
                    ElementCursor += AdvanceLength;
                };
                
                lua_pop(L, 1);
                continue;
            };
        
        /* SHOULD NEVER HAPPEN */
        } else {
            return luaL_error(L, "Impossible condition encountered in 'ScreenBuf:WriteRegion': Tried to output argument of illegal type!");
        };
    };};
    
    lua_pushboolean(L, Success);
    return 1;
};

static int lwcsbm_WriteRun(lua_State *L) {
    HANDLE       ThisHandle;
    unsigned int NumArgs;
    COORD        ThisPos;
    COORD        ThisRealSize;
    BOOL         Success = TRUE;
    
    WORD         CurrentAttrib;
    BOOL         UsingAttrib = FALSE;
    unsigned int Cursor;
    
    unsigned int RowLength; /* TODO: Again, this could probably be replaced with a #define. */
    
    NumArgs = lua_gettop(L);
    if (NumArgs < 2) {
        return luaL_error(L, "Invalid number of arguments to function 'ScreenBuf:WriteRun', please pass at least 1 argument!");
    };
    
    if (!lwcu_IsAScreenBuffer(L, 1)) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:WriteRun");
    } else {
        ThisHandle = ((ScreenBufObj*)lua_touserdata(L, 1))->Handle;
    };
    
    if (!lwcu_IsACoord(L, 2)) {
        return luaL_error(L, NotACoordErrString, 1, "ScreenBuf:WriteRun");
    } else if (!lwcu_IsAPositionCoord(L, 2)) {
        return luaL_error(L, NotAPositionCoordErrString, 1, "ScreenBuf:WriteRun");
    } else {
        ThisPos = lwcu_ToCoord(L, 2, -1);
    };
    
    /* Now we first loop through all arguments in an attempt to verify them. */
    if (NumArgs > 2) {
        unsigned int idx;
        for (idx = 3; idx <= NumArgs; idx++) {
            if        (lua_isnil(L, idx)) {
                continue;
            } else if (lwcu_IsAnAttrib(L, idx)) {
                /* It is explicitly required that we check for being an attrib, before checking for being a number or string. */
                continue;
            } else if (lua_type(L, idx) == LUA_TNUMBER) {
                if (lua_tointeger(L, idx) < 0) {
                    return luaL_error(L, "Invalid argument #%d to function 'ScreenBuf:WriteRun': number outside legal range!",
                        idx - 1);
                } else {
                    continue;
                };
            } else if (lua_type(L, idx) == LUA_TSTRING) {
                continue;
            } else { /* Not one of our acknowledged valid types. */
                return luaL_error(L, "Invalid argument #%d to function 'ScreenBuf:WriteRun': not a nil, attrib, number, or string!",
                    idx - 1);
            };
        };
    } else {
        /* If we have nothing in our variadic section, just return true. */
        lua_pushboolean(L, TRUE);
        return 1;
    };
    
    {CONSOLE_SCREEN_BUFFER_INFO ThisInfo;
        if (!GetConsoleScreenBufferInfo(ThisHandle, &ThisInfo)) {
            lua_pushboolean(L, FALSE);
            return 1;
        };
        ThisRealSize = ThisInfo.dwSize;
    };
    
    /* Check to see that our write position is valid. */
    if ((ThisPos.X >= ThisRealSize.X) || (ThisPos.Y >= ThisRealSize.Y)) {
        lua_pushboolean(L, FALSE);
        return 1;
    };
    
    Cursor = (ThisPos.Y * ThisRealSize.X) + ThisPos.X; /* This cursor is relative to the whole screenbuffer. */
    RowLength = ThisRealSize.X;
    
    {unsigned int idx;
     unsigned int FirstInvalidCursor = ThisRealSize.X * ThisRealSize.Y; /* This FirstInvalidCursor is relative to the whole screenbuffer. */
     for (idx = 3; (idx <= NumArgs) && (Cursor < FirstInvalidCursor); idx++) {
        
        /* NIL HANDLER */
        if        (lua_isnil(L, idx)) {
            UsingAttrib = FALSE;
            continue;
        
        /* ATTRIB HANDLER */
        } else if (lwcu_IsAnAttrib(L, idx)) {
            UsingAttrib = TRUE;
            CurrentAttrib = lwcu_ToAttrib(L, idx);
            continue;
        
        /* NUMBER HANDLER */
        } else if (lua_type(L, idx) == LUA_TNUMBER) {
            lua_Integer ThisNum = lua_tointeger(L, idx);
            
            if (!UsingAttrib) {
                Cursor += ThisNum;
                continue;
            } else {
                COORD WritePos;
                DWORD dummy;
                DWORD CellsToWrite = (ThisNum > (FirstInvalidCursor - Cursor)) ?
                                        FirstInvalidCursor - Cursor :
                                        ThisNum;
                
                WritePos.X = Cursor % RowLength;
                WritePos.Y = Cursor / RowLength;
                
                Success |= (TRUE ^ FillConsoleOutputAttribute(
                                        ThisHandle,
                                        CurrentAttrib,
                                        CellsToWrite,
                                        WritePos,
                                        &dummy));
                Cursor += CellsToWrite;
                continue;
            };
        
        } else if (lua_type(L, idx) == LUA_TSTRING) {
            size_t  CellsToWrite;
            LPCWSTR ThisStr = luaW_tolstring(L, idx, &CellsToWrite);
            COORD   WritePos;
            DWORD   dummy;
            
            WritePos.X = Cursor % RowLength;
            WritePos.Y = Cursor / RowLength;
            
            if (CellsToWrite > (FirstInvalidCursor - Cursor)) {
                CellsToWrite = FirstInvalidCursor - Cursor;
            };
            
            if (CellsToWrite > 0) {
                Success |= (TRUE ^ WriteConsoleOutputCharacterW(
                                        ThisHandle,
                                        ThisStr,
                                        CellsToWrite,
                                        WritePos,
                                        &dummy));
                if (UsingAttrib) {
                    Success |= (TRUE ^ FillConsoleOutputAttribute(
                                        ThisHandle,
                                        CurrentAttrib,
                                        CellsToWrite,
                                        WritePos,
                                        &dummy));
                };
            };
            
            Cursor += CellsToWrite;
            continue;
        
        /* SHOULD NEVER HAPPEN */
        } else {
            return luaL_error(L, "Impossible condition encountered in 'ScreenBuf:WriteRun': Tried to output argument of illegal type!");
        };
    };};
    
    lua_pushboolean(L, Success);
    return 1;
};

/* IMPLEMENTATION-SPECIFIC METHOD */
static int lwcsbm_SetColorTable(lua_State *L) {
    HANDLE ThisHandle;
    CONSOLE_SCREEN_BUFFER_INFOEX ThisInfo;
    
    ThisInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX); /* Necessary for GetConsoleScreenBufferInfoEx() */
    
    /* Check to make sure that both of our required functions are available. */
    if (!(ColorFuncsAreAvailable)) {
        lua_pushnil(L);
        return 1;
    };
    
    if (lua_gettop(L) != 2) {
        return luaL_error(L, NumArgsErrString, "ScreenBuf:SetColorTable", 1);
    };
    
    if (!lwcu_IsAScreenBuffer(L, 1)) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:SetColorTable");
    } else {
        ThisHandle = ((ScreenBufObj*)lua_touserdata(L, 1))->Handle;
    };
    
    if (!lwcu_IsAColorArray(L, 2)) {
        return luaL_error(L, NotAColorArrayErrString, 1, "ScreenBuf:SetColorTable");
    };
    
    GetConsoleScreenBufferInfoEx(ThisHandle, &ThisInfo);
    ThisInfo.srWindow.Right += 1;
    ThisInfo.srWindow.Bottom += 1;
    
    lwcu_ToColorArray(L, 2, ThisInfo.ColorTable);
    
    lua_rawgeti(L, UPV_RootTable, RE_ColorTblLookup);
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_settable(L, -3);
    
    lua_pushboolean(L, SetConsoleScreenBufferInfoEx(ThisHandle, &ThisInfo));

    return 1;
};

/* FOR INVESTIGATORY PURPOSES ONLY; IMPLEMENTATION-SPECIFIC FUNCTION */
#if 0 /* Please don't re-active this function. It was designed even before lwcu_PushArray() and lwcu_ToArray(). */
static int lwcsbm_GetColorTable(lua_State *L) {
    HANDLE ThisHandle;
    CONSOLE_SCREEN_BUFFER_INFOEX ThisInfo;
    
    ThisInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    
    if (GetConsoleScreenBufferInfoEx == NULL) {
        lua_pushnil(L);
        return 1;
    };
    
    if (lua_gettop(L) != 1) {
        return luaL_error(L, NumArgsErrString, "ScreenBuf:GetColorTable", 0);
    };
    
    if (!lwcu_IsAScreenBuffer(L, 1)) {
        return luaL_error(L, NotAScreenBufferErrString, 0, "ScreenBuf:GetColorTable");
    } else {
        ThisHandle = ((ScreenBufObj*)lua_touserdata(L, 1))->Handle;
    };
    
    if (!GetConsoleScreenBufferInfoEx(ThisHandle, &ThisInfo)) {
        lua_pushboolean(L, FALSE);
        return 1;
    };
    
    lua_createtable(L, 16, 0); /* At stack index 2 */
    
    {int idx;
     for (idx = 1; idx <= 16; idx++) {
        COLORREF ThisCol = ThisInfo.ColorTable[idx-1];
        
        lua_createtable(L, 0, 3); /* At stack index 3 */
        
        lua_pushinteger(L, GetRValue(ThisCol));
        lua_setfield(L, 3, "r");
        
        lua_pushinteger(L, GetGValue(ThisCol));
        lua_setfield(L, 3, "g");
        
        lua_pushinteger(L, GetBValue(ThisCol));
        lua_setfield(L, 3, "b");
        
        lua_rawseti(L, 2, idx);
    };};

    return 1;
};
#endif

static int lwcsbm_gc(lua_State *L) {
    CloseHandle(((ScreenBufObj*)lua_touserdata(L, 1))->Handle);

    return 0;
};