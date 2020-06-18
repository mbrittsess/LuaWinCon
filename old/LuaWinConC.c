/* * * * * * * * * * * * * * * * *
 *     Created February 2012     *
 *  by Matthew Britton Sessions  *
 *     Designed for Lua 5.2      *
 * * * * * * * * * * * * * * * * *
 *  Best viewed at 140 columns.  *
 * * * * * * * * * * * * * * * * */

#include "LuaWinCon.h"

static int lw32c_SetTitle(lua_State *L) {
    /* LuaWinCon.SetTitle
    Parameters:
       Title   : string
    Returns:
       Success : boolean
    Description:
        Takes a string as input and sets it as the title of the console window.
     The string cannot contain embedded zeroes. Returns true on success and 
     false on failure.
    Error conditions:
        Function errors if first parameter is not a string or convertible to
     a string.
    */
    
    if (!lua_isstring(L, 1)) {
        return luaL_error(L, "Bad argument #1 to SetTitle(), string expected!");
    };
    
    lua_pushboolean(L, SetConsoleTitle(luaW_tostring(L, 1)));
    
    return 1;
};

static int lw32c_SetBufferDimensions(lua_State *L) {
    /* LuaWinCon.SetBufferDimensions
    Parameters:
       B       : buffer
       Size    : coord
    Returns:
       Success : boolean
    Description:
        Sets the dimensions of the given buffer. Returns true on success and
     false on failure. Is a valid buffer method.
    Error conditions:
        Function errors if 'B' is not a valid screen buffer handle, or if
     'Size' is not a valid coord.
    */
    
    if        (!lw32ci_IsValidBuffer(L, 1)) {
        return luaL_error(L, "Bad argument #1 to SetBufferDimensions(), buffer expected!");
    } else if (!lw32ci_IsValidCoord(L, 2)) {
        return luaL_error(L, "Bad argument #2 to SetBufferDimensions(), coord expected!");
    };
    
    lua_pushboolean(L, SetConsoleScreenBufferSize(
                        lw32ci_ToHANDLE(L, 1),
                        lw32ci_ToCOORD(L, 2, 1)));
    
    return 1;
};

static int lw32c_SetWindowPosition(lua_State *L) {
    /* LuaWinCon.SetWindowPosition
    Parameters:
       B        : buffer
       Position : coord
       Size     : coord
    Returns:
       Success  : boolean
    Description:
        Sets the windowed view of the buffer 'B' so that the upper-left
     corner is at 'Position' in the buffer, and the window is 'Size' in size.
     Returns true on success and false on failure. It is a valid buffer
     method.
    Error conditions:
        Function errors if 'B' is not a valid screen buffer handle, or if
     'Position' or 'Size' are not valid coords.
    */
    
    if        (!lw32ci_IsValidBuffer(L, 1)) {
        return luaL_error(L, "Bad argument #1 to SetWindowPosition(), buffer expected!");
    } else if (!lw32ci_IsValidCoord(L, 2)) {
        return luaL_error(L, "Bad argument #2 to SetWindowPosition(), coord expected!");
    } else if (!lw32ci_IsValidCoord(L, 3)) {
        return luaL_error(L, "Bad argument #3 to SetWindowPosition(), coord expected!");
    };
    
    COORD ThisPosition = lw32ci_ToCoord(L, 2, 0);
    COORD ThisSize     = lw32ci_ToCoord(L, 3, 1);
    const SMALL_RECT ThisRect = {
        ThisPosition.X,
        ThisPosition.Y,
        ThisPosition.X + ThisSize.X,
        ThisPosition.Y + ThisSize.Y};
    
    lua_pushboolean(L, SetConsoleWindowInfo(
                        lw32ci_ToHandle(L, 1),
                        TRUE,
                        &ThisRect));
    
    return 1;
};

static int lw32c_MakeActiveBuffer(lua_State *L) {
    /* LuaWinCon.MakeActiveBuffer
    Parameters:
       B       : buffer
    Returns:
       Success : boolean
    Description:
        Sets the active screen buffer to 'B'. Returns true on success and false
     on failure. It is a valid buffer method.
    Error conditions:
        Function errors if 'B' is not a valid screen buffer handle.
    */
    
    if (!lw32ci_IsValidBuffer(L, 1)) {
        return luaL_error(L, "Bad argument #1 to MakeActiveBuffer(), buffer expected!");
    } else {
        lua_pushboolean(L, SetConsoleActiveScreenBuffer(lw32ci_ToHandle(L, 1)));
    };
    
    return 1;
};

static int lw32c_CreateNewBuffer(lua_State *L) {
    /* LuaWinCon.CreateNewBuffer
    Returns:
       Handle : buffer / nil
    Description:
        Returns a new buffer handle, or a nil if one could not be obtained. If
     no buffer handles are currently alive in the Lua state, it returns the 
     handle for the current active buffer.
    */
    HANDLE ThisHandle;
    
    lua_pushnil(L);
    if (lua_next(L, lua_upvalueindex(1)) == 0) { /* No buffers currently alive. */
        ThisHandle = CreateFileW(
                        L"CONOUT$",
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL); /* All values as recommended by the MSDN. */
    } else {
        ThisHandle = CreateConsoleScreenBuffer(
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        CONSOLE_TEXTMODE_BUFFER,
                        NULL);
    };
    
    if (ThisHandle == INVALID_HANDLE_VALUE) {
        lua_pushnil(L);
        return 1;
    };
    
    lw32ci_PushHandle(L, ThisHandle);
    lua_pushvalue(L, lua_upvalueindex(2)); /* Push buffer metatable onto stack, */
    lua_setmetatable(L, -2);               /* then set it to the buffer. */
    
    lua_pushvalue(L, -1);                  /* Now make a copy of the handle, */
    lua_pushboolean(L, 1);                 /* then a 'true', */
    lua_settable(L, lua_upvalueindex(1));  /* and put them into our buffer-life table. */
    
    /* Buffer is at top of stack. */
    return 1;
};

static int lw32c_GetLargestWindowDimensions(lua_State *L) {
    /* LuaWinCon.GetLargestWindowDimensions
    Parameters:
       B    : buffer
    Returns:
       Size : coord
    Description:
        Returns a coord describing the current largest size a viewing window
     could be for a given buffer,, given any number of characteristics about
     the system and its configuration at the time. It is a valid buffer method.
    Error conditions:
        Function will error if passed an invalid buffer.
    */
    
    if (!lw32ci_IsValidBuffer(L, 1)) {
        return luaL_error("Bad argument #1 to GetLargestWindowDimensions(), buffer expected!");
    };
    
    lw32ci_PushCoord(L, GetLargestConsoleWindowSize(lw32ci_ToHandle(L, 1)), 1);
    return 1;
};

/* 
DEAR GOD DON'T DO THIS
Just give us the ability to save a userdatum which holds the state of
the console, and which we can restore to when we want!
*/
static int lw32c_SetInputEnabled(lua_State *L) {
    /* LuaWinCon.SetInputEnabled
    Parameters:
       Command : boolean
    Returns:
       Success : boolean
    Description:
        Controls the input and output mode of the console and controls access
     to the input functionality. If 'Command' is true, then all high-level
     console modes are disabled, the input buffer is flushed, the current 
     console mode is saved if there is no 'live' data already, the input
     functions will start returning data, and the input handle for the console
     is fetched anew.
        If 'Command' is false, then the console mode is restored to a prior
     state, the input functions will stop returning data, and the input handle
     is closed.
        If the function succeeds, it also clears the input buffer of all event
     records.
        The return values indicate success at setting the flags as appropriate
     and entering the new state. If the program is already in a disabled state
     and false is given for 'Command', the return value will also be false.
     
                             - DEVELOPER NOTES -
        There is an important note regarding the behavior of 'saving' and
     'restoring' the console modes. When 'Command' is true, the function will
     attempt to save the current mode flags to an upvalue, *provided* that this
     upvalue does not contain any existing mode flags. Conversely, when
     'Command' is false, the console mode will be set to the upvalue's flags,
     provided that it contains any...and if it does, after setting them, the
     upvalue is set to an empty state.
        While this behavior is perhaps a bit awkward, when designing this
     function, I realized that there was no true "default" state for console
     mode. In keeping with the philosophy of this library, the console should
     be in as low a level as possible when actively being used, however, the
     library is also supposed to be unobtrusive and not make permanent changes
     to the console state which cannot be blindly undone.
        Therefore, I arrived at this current system, as I believe it most
     satisfies the ability for someone to begin using the library at any point
     in a program and be able to return to his prior state from any point,
     whilst requiring only a modicum of thought to be put into the exact points
     at which to enable and disable input.
        A corollary of this, is that if the function is unable to save the
     current mode when input is being enabled, it will not carry through the
     action and the function will fail.
    */
    
    #define ConsoleInputHandleIndex lua_upvalueindex(1)
    #define OriginalModeFlagsIndex  lua_upvalueindex(2)
    #define ModeFlagsAreLiveIndex   lua_upvalueindex(3)
    
    DWORD  TheseFlags;
    HANDLE ThisHandle;
    
    if (!lua_isboolean(L, 1)) {
        return luaL_error(L, "Bad argument #1 to SetInputEnabled(), boolean expected!");
    } else if (lua_toboolean(L, 1)) {                                       /* COMMAND == TRUE */
        if (lua_toboolean(L, ModeFlagsAreLiveIndex)) {                       /* ALREADY ENABLED */
            ThisHandle = lw32ci_ToHandle(L, ConsoleInputHandleIndex);
            lua_pushboolean(L, SetConsoleMode(ThisHandle, MOUSE_INPUT_ENABLED));
            FlushConsoleInputBuffer(ThisHandle);
            return 1;
        } else {                                                             /* NOT ENABLED YET */
            ThisHandle = CreateFileW(
                            L"CONIN$",
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);
            /* We will catch a failure to create the handle when we try to use it. */
            if (!GetConsoleMode(ThisHandle, &TheseFlags)) {                   /* FAILED TO FETCH OLD FLAGS */
                CloseHandle(ThisHandle);
                lua_pushboolean(L, 0);
                return 1;
            } else {                                                          /* SUCCESSFULLY FETCHED OLD FLAGS */
                if (!SetConsoleMode(ThisHandle, MOUSE_INPUT_ENABLED)) {        /* FAILED TO SET NEW FLAGS */
                    CloseHandle(ThisHandle);
                    lua_pushboolean(L, 0);
                    return 1;
                } else {                                                       /* SUCCESSFULLY SET NEW FLAGS */
                    lua_pushboolean(L, 1);
                    lw32ci_PushHandle(L, ThisHandle);
                    lua_pushinteger(L, TheseFlags);
                    lua_pushboolean(L, 1);
                    
                    lua_replace(L, ModeFlagsAreLiveIndex);
                    lua_replace(L, OriginalModeFlagsIndex);
                    lua_replace(L, ConsoleInputHandleIndex);
                    FlushConsoleInputBuffer(ThisHandle);
                    return 1;
                };
            };
        };
    } else {                                                                /* COMMAND == FALSE */
        if (!lua_toboolean(L, ModeFlagsAreLiveIndex)) {                      /* ALREADY DISABLED */
            lua_pushboolean(L, 0);
            return 1;
        } else {                                                             /* NOT DISABLED YET */
            ThisHandle = lw32ci_ToHandle(L, ConsoleInputHandleIndex);
            TheseFlags = lua_tointeger(L, OriginalModeFlagsIndex);
            
            if (!SetConsoleMode(ThisHandle, TheseFlags)) {                    /* FAILED TO RESTORE FLAGS */
                lua_pushboolean(L, 0);
                return 1;
            } else {                                                          /* SUCCESSFULLY RESTORED FLAGS */
                CloseHandle(ThisHandle);
                lua_pushboolean(L, 0);
                lua_replace(L, ModeFlagsAreLiveIndex);
                lua_pushboolean(L, 1);
                FlushConsoleInputBuffer(ThisHandle);
                return 1;
            };
        };
    };
    
    #undef ConsoleInputHandleIndex
    #undef OriginalModeFlagsIndex
    #undef ModeFlagsAreLiveIndex
};

static int lw32c_ClearInput(lua_State *L) {
    /* LuaWinCon.ClearInput
    Parameters:
    Returns:
    Description:
    */
    
    #define ConsoleInputHandleIndex lua_upvalueindex(1)