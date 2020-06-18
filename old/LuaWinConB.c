/* * * * * * * * * * * * * * 
    Created February 2012
 by Matthew Britton Sessions
    Designed for Lua 5.2
 * * * * * * * * * * * * * */

#include <lua.h>
#include <lauxlib.h>
#include <windows.h>

#define lua_print(L, s) \
    lua_getglobal(L, "print"); \
    lua_pushlstring(L, "" s, (sizeof(s)/sizeof(char))-1); \
    lua_call(L, 1, 0)

/* FORWARD DECLARATIONS */
static const char *luaW_pushstring(lua_State *L, LPCWSTR s);
#define luaW_pushliteral(L, s)	\
	lua_pushlstring(L, (const char *)L"" s, (sizeof(s)/sizeof(WCHAR))-1)
static int lw32c_internal_UTF8toUTF16(lua_State *L);
static int lw32c_GetLargestConsoleSize(lua_State *L);
static int lw32c_SetConsoleSize(lua_State *L);
static int lw32c_SetTitle(lua_State *L);
static BOOL lw32c_internal_CheckVersion(void);
/*static int lw32c_GetColorTable(lua_State *L);*/
/*static int lw32c_SetColorTable(lua_State *L);*/
static int lw32c_internal_IsValidCoord(lua_State *L, int idx);
static int lw32c_internal_IsValidAttrib(lua_State *L, int idx);
static WORD lw32c_internal_AttribToWORD(lua_State *L, int idx);
static int lw32c_FillRegion(lua_State *L);
static int lw32c_WriteRegion(lua_State *L);
static int lw32c_WriteRun(lua_State *L);
static int lw32c_SetCursorVisible(lua_State *L);
static int lw32c_SetCursorPosition(lua_State *L);
static int lw32c_SetCursorSize(lua_State *L);
static COORD lw32c_internal_ToCOORD(lua_State *L, int idx, int one_based);
static unsigned int lw32c_internal_COORDtoOffset(COORD point, COORD size);
static COORD lw32c_internal_OffsetToCOORD(unsigned int ofs, COORD size);
static int lw32c_GetNumberOfMouseButtons(lua_State *L);
static int lw32c_LowLevelMode(lua_State *L);
static int lw32c_ClearInput(lua_State *L);
static int lw32c_ReadInput(lua_State *L);
static int lw32c_PeekInput(lua_State *L);
static void lw32c_internal_PushKeyTable(lua_State *L, KEY_EVENT_RECORD* K);
static int lw32c_internal_WHCARtoUTF8(WCHAR InChar, char* OutChar);
static void lw32c_internal_PushMouseTable(lua_State *L, MOUSE_EVENT_RECORD* M);
/*static int lw32c_Beep1(lua_State *L);*/
/*static int lw32c_Beep2(lua_State *L);*/
/*static int lw32c_Beep3(lua_State *L);*/
/*static int lw32c_GetClipboardText(lua_State *L);*/
/*static int lw32c_SetClipboardText(lua_State *L);*/
int luaopen_LuaWinCon(lua_State *L);
/* END FORWARD DECLARATIONS */

/* SOME NOTES
    Until I decide on exactly how I want to deal with multiple console buffers, I'm going to enclose any 
  code which returns a HANDLE to a console screen buffer in comments containing "AA" and "AB".
   END NOTES */
   
static const char *luaW_pushstring(lua_State *L, LPCWSTR s) {
    /* This function takes a nul-terminated UTF-16 string, and pushes it on the stack as a UTF-8 string. 
    It returns a pointer to Lua's internal copy of the string. The memory at 's' can be freed or reused
    immediately after the function returns. 
    */
    HANDLE ThisHeap = GetProcessHeap();
    LPSTR  ThisUTF8;
    int    ThisUTF8siz = WideCharToMultiByte(
        CP_UTF8,
        0,
        s,
        -1,
        NULL,
        0,
        NULL,
        NULL);
    
    ThisUTF8 = (LPSTR)HeapAlloc(
        ThisHeap,
        0, /* Can't quite be sure if it's safe to use HEAP_NO_SERIALIZE in all possible future cases */
        ThisUTF8siz);
    
    WideCharToMultiByte(
        CP_UTF8,
        0,
        s,
        -1,
        ThisUTF8,
        ThisUTF8siz,
        NULL,
        NULL);
    
    HeapFree(ThisHeap, 0, ThisUTF8);
    
    return lua_pushstring(L, ThisUTF8);
};

static int lw32c_internal_UTF8toUTF16(lua_State *L) {
    /* This function is used only internally, it is not exported.
    It takes a single argument, a UTF-8 string, and returns a single value, a UTF-16 string.
    */
    HANDLE ThisHeap = GetProcessHeap();
    size_t cbUTF8siz;
    LPCSTR UTF8 = lua_tolstring(L, 1, &cbUTF8siz);
    LPWSTR ThisUTF16;
    int  cchThisUTF16siz = MultiByteToWideChar(
        CP_UTF8,
        0,
        UTF8,
        cbUTF8siz,
        NULL,
        0);
    
    ThisUTF16 = (LPWSTR)HeapAlloc(ThisHeap, 0, cchThisUTF16siz * sizeof(WCHAR));
    MultiByteToWideChar(
        CP_UTF8,
        0,
        UTF8,
        cbUTF8siz,
        ThisUTF16,
        cchThisUTF16siz);
    
    lua_pushlstring(L, (LPCSTR)ThisUTF16, cchThisUTF16siz * sizeof(WCHAR));
    
    HeapFree(ThisHeap, 0, ThisUTF16);
    
    return 1;
};
    
static int lw32c_GetLargestConsoleSize(lua_State *L) {
    /* LuaWinCon.GetLargestConsoleSize()
    Returns the size, in characters, of the largest width, and the largest height, that a console can currently have.
    */
    COORD ThisSiz = GetLargestConsoleWindowSize(/*AA*/GetStdHandle(STD_OUTPUT_HANDLE)/*AB*/);
    
    lua_createtable(L, 2, 0);
    lua_pushinteger(L, (lua_Integer)ThisSiz.X);
    lua_rawseti(L, -2, 1);
    lua_pushinteger(L, (lua_Integer)ThisSiz.Y);
    lua_rawseti(L, -2, 2);
    
    return 1;
};

static int lw32c_SetConsoleSize(lua_State *L) {
    /* LuaWinCon.SetConsoleSize(size)
        Sets the console to be 'size' in number of columns and rows. 'size' is a 'coord'-type table.
        Returns a true if the operation was successful, false otherwise.
    */
    
    if (lw32c_internal_IsValidCoord(L, 1) != 1) {
        lua_pushboolean(L, 0);
    } else {
        SMALL_RECT ThisRect;
        COORD      ThisSiz;
        HANDLE     ThisHandle = /*AA*/GetStdHandle(STD_OUTPUT_HANDLE)/*AB*/;
    
        lua_pushinteger(L, 2);
        lua_gettable(L, 1);
        lua_pushinteger(L, 1);
        lua_gettable(L, 1);
        
        ThisSiz.X = (SHORT)lua_tounsigned(L, -1);
        ThisSiz.Y = (SHORT)lua_tounsigned(L, -2);
        
        ThisRect.Left   = 0;
        ThisRect.Top    = 0;
        ThisRect.Right  = ThisSiz.X - 1;
        ThisRect.Bottom = ThisSiz.Y - 1;
        
        lua_pushboolean(
            L, 
            SetConsoleWindowInfo(ThisHandle, TRUE, &ThisRect
            ) | SetConsoleScreenBufferSize(ThisHandle, ThisSiz)
            );
        SetConsoleWindowInfo(ThisHandle, TRUE, &ThisRect); /* If we upsized, then we have to call this *after* buffer re-sizing. */
    };

    return 1;
};

/* Check if this really needs to be stored externally like this. */
LPWSTR TitleString = NULL;
static int lw32c_SetTitle(lua_State *L) {
    /* LuaWinCon.SetTitle(v)
    Sets the title for the console window. 'v' is expected to be UTF-8.
    Returns a boolean to indicate success or failure.
    */
    LPCSTR  InternalString;
    size_t  cbInternalStringSiz;
    LPWSTR NewTitleString;
    HANDLE ThisHeap = GetProcessHeap();
    const WCHAR ThisNul = 0;
    
    lua_pushcfunction(L, lw32c_internal_UTF8toUTF16);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);
    lua_pushlstring(L, (const char *)&ThisNul, sizeof(WCHAR)); /*Lua only guarantees an ASCII NUL at the end, we need a UTF-16 NUL. */
    lua_concat(L, 2);
    
    InternalString = lua_tolstring(L, -1, &cbInternalStringSiz);
    NewTitleString = HeapAlloc(ThisHeap, 0, cbInternalStringSiz);
    CopyMemory(NewTitleString, InternalString, cbInternalStringSiz);
    
    lua_pushboolean(L, SetConsoleTitleW((LPCWSTR)NewTitleString));
    HeapFree(ThisHeap, 0, TitleString);
    TitleString = NewTitleString;
    
    return 1;
};

static BOOL lw32c_internal_CheckVersion(void) {
    /* Returns TRUE if the OS is Windows Vista or Windows Server 2008 or greater. */
    OSVERSIONINFOEX ThisInfo = {
        sizeof(OSVERSIONINFOEX),
        6, /* Major Version must be >= 6 */
        0, /* Minor Version must be >= 0 */
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0};
    DWORDLONG ConditionMask = 0;
    
    /* While we only want to check the Major Version and Minor Version, the documentation indicates that if you check those, you are
    required to also check the Service Pack Major and Minor Versions. */
    VerSetConditionMask(ConditionMask, VER_MAJORVERSION,     VER_GREATER_EQUAL);
    VerSetConditionMask(ConditionMask, VER_MINORVERSION,     VER_GREATER_EQUAL);
    VerSetConditionMask(ConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
    VerSetConditionMask(ConditionMask, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);
        
    return VerifyVersionInfo(
        &ThisInfo,
        VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        ConditionMask);
};

#if 0 /* Deleting this section of code for now. */
/* TODO TODO TODO
    These functions...well...the color table values don't seem to actually...do much.
    It bears investigation...particularly on Windows XP computers. */
static int lw32c_GetColorTable(lua_State *L) {
    CONSOLE_SCREEN_BUFFER_INFOEX ThisInfo;

    if (lw32c_internal_CheckVersion()) {
        /* TODO: THIS NEXT LINE IS ONLY FOR NOW, WE WILL BUILD A DEFAULT TABLE LATER */
        lua_pushboolean(L, 0);
    } else {
        GetConsoleScreenBufferInfoEx(
            /*AA*/GetStdHandle(STD_OUTPUT_HANDLE)/*AB*/,
            &ThisInfo);
        lua_createtable(L, 16, 0);
        {int i;
        for (i = 0; i < 16; i++) {
            COLORREF ThisColor = ThisInfo.ColorTable[i];
            const unsigned char ThisColorString[3] = {GetRValue(ThisColor), GetGValue(ThisColor), GetBValue(ThisColor)};
            lua_pushlstring(L, ThisColorString, 3);
            lua_rawseti(L, -2, i+1);
        };};
    };
            
    return 1;
};
#endif

#if 0
static int lw32c_SetColorTable(lua_State *L) {
    return 1;
};
#endif
/* END TODO */

static int lw32c_internal_IsValidCoord(lua_State *L, int idx) {
    /* Returns 1 if the value at the specified index can be used as a coordinate, 0 otherwise. */
    #define Win32_SHORT_max 32767 /* As given in "Windows Data Types" in the MSDN. */
    
    if (!lua_istable(L, idx)) {
        if (luaL_getmetafield(L, idx, "__index")) {
            lua_pop(L, 1);
        } else {
            return 0;
        };
    };
    {int i;
     lua_Unsigned val;
     for (i = 1; i <= 2; i++) {
        lua_pushinteger(L, i);
        lua_gettable(L, idx);
        val = lua_tounsigned(L, -1);
        lua_pop(L, 1);
        if ((val == 0) || (val > Win32_SHORT_max)) {
            return 0;
        };
    };};
    
    #undef Win32_SHORT_max
    return 1;
};

static int lw32c_internal_IsValidAttrib(lua_State *L, int idx) {
    /* Like IsValidCoord, but for Attributes. */
    
    if (!lua_istable(L, idx)) {
        if (luaL_getmetafield(L, idx, "__index")) {
            lua_pop(L, 1);
        } else {
            return 0;
        };
    };
    {int i;
     unsigned int j;
     for (i = 1; i <= 2; i++) {
        lua_pushinteger(L, i);
        lua_gettable(L, idx);
        j = lua_tounsigned(L, -1);
        if ((j < 1) || (j > 16)) {
            lua_pop(L, 1);
            return 0;
        };
        lua_pop(L, 1);
    };};
    
    return 1;
};

static WORD lw32c_internal_AttribToWORD(lua_State *L, int idx) {
    WORD Result;
    
    lua_pushinteger(L, 1);
    lua_gettable(L, idx);
    lua_pushinteger(L, 2);
    lua_gettable(L, idx);
    Result = ((lua_tounsigned(L, -2) - 1) | ((lua_tounsigned(L, -1) - 1) << 4));
    lua_pop(L, 2);
    
    return Result;
};

static int lw32c_FillRegion(lua_State *L) {
    /* LuaWinCon.FillRegion(origin, size, [attrib,] [char])
        PARM NOTE: The 'attrib' and 'char' can come in either order, and there must be at least one.
    Fills the region with the top-left corner at 'origin' and 'size' in size, with the attribute 'attrib' if given, or leaving
    untouched if not, and as many copies as are necessary of character 'char' if given, or leaving untouched if not.
    Returns a true if the operation was successful, false otherwise.
    */
    /* First check that we've got sufficient arguments */
    WCHAR ThisWChar  = 0;
    int   ThisWCharFound = 0;
    WORD ThisAttrib = 0xFF + 1; /* Impossible value, 256 */
    COORD Origin;
    COORD Size;
    BOOL TempResult;
    BOOL Result = TRUE;
    
    if (lua_gettop(L) < 3) {
        lua_pushliteral(L, "Insufficient arguments to FillRegion()!");
        return lua_error(L);
    };
    
    /* Next check to see that our first two arguments are both coords. */
    {int arg;
     for(arg = 1; arg <= 2; arg++) {
        if (!lw32c_internal_IsValidCoord(L, arg)) {
            return luaL_error(L, "Bad argument #%d to FillRegion: Not a valid 'coord'!", arg);
        };
    };};
    
    /* Now we'll check to see that our third and maybe fourth arguments are correct, and assign them as necessary. */
    {int arg;
     for(arg = 3; arg <= 4; arg++) {
        if ((lw32c_internal_IsValidAttrib(L, arg)) && (ThisAttrib == 256)) {
            ThisAttrib = lw32c_internal_AttribToWORD(L, arg);
        } else if ((lua_isstring(L, arg)) && (ThisWCharFound == 0)) {
            if (lua_rawlen(L, arg) > 0) {
                lua_pushcfunction(L, lw32c_internal_UTF8toUTF16);
                lua_pushvalue(L, arg);
                lua_call(L, 1, 1);
                ThisWChar = ((LPCWSTR)lua_tostring(L, -1))[0];
            };
            ThisWCharFound = 1;
        } else {
            return luaL_error(L, "Bad argument #%d to FillRegion: Not a string or valid 'attrib'!", arg);
        };
    };};
    
    /* One last thing to take care of, assigning values to the Origin and Size COORDs. */
    lua_pushinteger(L, 1);
    lua_gettable(L, 1);
    Origin.X = (SHORT)(lua_tointeger(L, -1) - 1);
    lua_pushinteger(L, 2);
    lua_gettable(L, 1);
    Origin.Y = (SHORT)(lua_tointeger(L, -1) - 1);
    
    lua_pushinteger(L, 1);
    lua_gettable(L, 2);
    Size.X = (SHORT)lua_tointeger(L, -1);
    lua_pushinteger(L, 2);
    lua_gettable(L, 2);
    Size.Y = (SHORT)lua_tointeger(L, -1);
    
    /* We might change how we do this in the future. */
    {SHORT yOfs;
     HANDLE ThisHandle = /*AA*/GetStdHandle(STD_OUTPUT_HANDLE)/*AB*/;
     DWORD Dummy;
     COORD ThisOrigin = {Origin.X, Origin.Y};
     if (ThisAttrib != 256) {
        for (yOfs = 0; yOfs < Size.Y; yOfs++) {
            ThisOrigin.Y = Origin.Y + yOfs; /* This isn't even unoptimized because COORD is passed by value, not reference. */
            TempResult = FillConsoleOutputAttribute(ThisHandle, ThisAttrib, Size.X, ThisOrigin, &Dummy);
            if (Result) {
                Result = TempResult;
            };
        };
     };
     if (ThisWCharFound == 1) {
        for (yOfs = 0; yOfs < Size.Y; yOfs++) {
            ThisOrigin.Y = Origin.Y + yOfs;
            TempResult = FillConsoleOutputCharacterW(ThisHandle, ThisWChar, Size.X, ThisOrigin, &Dummy);
            if (Result) {
                Result = TempResult;
            };
        };
     };
    };
    
    lua_pushboolean(L, Result);
    return 1;
};

static int lw32c_WriteRegion(lua_State *L) {
    /* LuaWinCon.WriteRegion(origin, size, {,attrib | string})
        Writes a series of attributes and strings. An attribute "sets" the current attribute. nil is a valid attribute,
    and causes attributes to not be written. A string will write out that string, wrapping at the region's boundaries.
    The empty string is a no-op. A nonnegative integer number will cause that many cells of attributes to be written.
        Any other values will trigger an error. The function will not attempt to write paste the defined box it is given.
        Returns a true if the operation was successful, false otherwise.
    */
    unsigned int    NumArgs;
    
    BOOL            UsingAttributes  = FALSE;
    WORD            CurrentAttribute = 0;
    
    CHAR_INFO*      CharInfoArray;
    unsigned int    ArrayLength;
    COORD           Origin;
    COORD           Size;
    SMALL_RECT      Region;
    
    LPCWSTR         CurrentString;
    size_t          StringLength;
    unsigned int    StringIndex = 0;
    
    unsigned int    CellIndex = 0;
    
    /* First check that we've got sufficient arguments */
    if (lua_gettop(L) < 3) {
        lua_pushliteral(L, "Insufficient arguments to WriteRegion()!");
        return lua_error(L);
    };
    
    NumArgs = lua_gettop(L);
    
    /* Next, check to see that our first two arguments are both valid coords. */
    {int arg;
     for (arg = 1; arg <= 2; arg++) {
        if (lw32c_internal_IsValidCoord(L, arg) == 0) {
            return luaL_error(L, "Bad argument #%d to WriteRegion: Not a valid 'coord'!", arg);
        };
    };};
    
    /* Now let's fill in our Origin and Size coords...and then remove those tables from the stack when we're done. */
    lua_pushinteger(L, 1);
    lua_gettable(L, 1);
    Origin.X = (SHORT)lua_tointeger(L, -1) - 1;
    lua_pushinteger(L, 2);
    lua_gettable(L, 1);
    Origin.Y = (SHORT)lua_tointeger(L, -1) - 1;
    lua_pop(L, 2);
    
    lua_pushinteger(L, 1);
    lua_gettable(L, 2);
    Size.X = (SHORT)lua_tointeger(L, -1);
    lua_pushinteger(L, 2);
    lua_gettable(L, 2);
    Size.Y = (SHORT)lua_tointeger(L, -1);
    lua_pop(L, 2);
    
    /* So let's start with setting up the array we'll be operating on. */
    ArrayLength   = Size.X * Size.Y;
    Region.Left   = Origin.X;
    Region.Top    = Origin.Y;
    Region.Right  = Origin.X + Size.X - 1;
    Region.Bottom = Origin.Y + Size.Y - 1;
    
    /* The actual "Origin" we pass to ReadConsoleOutput() and WriteConsoleOutput() actually specifies the origin in the copy
    of the CHAR_INFO arrays we use, so it should just be (0,0). */
    Origin.X = 0;
    Origin.Y = 0;
    
    CharInfoArray = (CHAR_INFO*)HeapAlloc(GetProcessHeap(), 0, ArrayLength * sizeof(CHAR_INFO));
    
    /* Read in our CHAR_INFO structures. */
    ReadConsoleOutputW(
        /*AA*/GetStdHandle(STD_OUTPUT_HANDLE)/*AB*/,
        CharInfoArray,
        Size,
        Origin,
        &Region);
    
    /* Time to start processing our arguments. */
    {int arg;
     for (arg = 3; arg <= NumArgs; arg++) {
        if        (lua_isnil(L, arg)) {
            UsingAttributes = FALSE;
        } else if (lw32c_internal_IsValidAttrib(L, arg)) {
            UsingAttributes = TRUE;
            CurrentAttribute = lw32c_internal_AttribToWORD(L, arg);
        } else if (lua_type(L, arg) == LUA_TSTRING) { /* Empty string is a no-op. */
            if (lua_rawlen(L, arg) == 0) {
                continue;
            };
            lua_pushcfunction(L, lw32c_internal_UTF8toUTF16);
            lua_pushvalue(L, arg);
            lua_call(L, 1, 1);
            CurrentString = (LPCWSTR)lua_tolstring(L, -1, &StringLength);
            StringLength = StringLength / sizeof(WCHAR); /* Length in chars to length in WCHARs. */
            StringIndex = 0;
            if (UsingAttributes) {
                while ((CellIndex < ArrayLength) && (StringIndex < StringLength)) {
                    CharInfoArray[CellIndex].Char.UnicodeChar = CurrentString[StringIndex];
                    CharInfoArray[CellIndex].Attributes = CurrentAttribute;
                    CellIndex += 1;
                    StringIndex += 1;
                };
            } else {
                while ((CellIndex < ArrayLength) && (StringIndex < StringLength)) {
                    CharInfoArray[CellIndex].Char.UnicodeChar = CurrentString[StringIndex];
                    CellIndex += 1;
                    StringIndex += 1;
                };
            };
            lua_pop(L, 1);
        } else if (lua_type(L, arg) == LUA_TNUMBER) {
            int value;
            lua_Number fractional;
            lua_Number orig_value = lua_tonumber(L, arg);
            
            lua_pushvalue(L, arg);
            lua_pushinteger(L, 1);
            lua_arith(L, LUA_OPMOD);
            fractional = lua_tonumber(L, -1);
            lua_pop(L, 1);
            
            /* Verify that it's sensible. */
            if (fractional != 0.0) {
                return luaL_error(L, "Bad argument #%d to WriteRegion(): Non-integer number!", arg);
            } else if (orig_value < 0) {
                return luaL_error(L, "Bad argument #%d to WriteRegion(): Negative number!", arg);
            };
            
            value = (int)orig_value;
            
            if (UsingAttributes) {
                while ((CellIndex < ArrayLength) && (value > 0)) {
                    CharInfoArray[CellIndex].Attributes = CurrentAttribute;
                    CellIndex += 1;
                    value -= 1;
                };
            } else {
                /* RE-DO THIS TO BE NOT-STUPID! */
                while ((CellIndex < ArrayLength) && (value > 0)) {
                    CellIndex += 1;
                    value -= 1;
                };
            };
        } else {
            luaL_error(L, "Bad argumnet #%d to WriteRegion(): Not a nil, attrib, string, or number!", arg);
        };
    };};
        
    lua_pushboolean(L, WriteConsoleOutputW(
        /*AA*/GetStdHandle(STD_OUTPUT_HANDLE)/*AB*/,
        CharInfoArray,
        Size,
        Origin,
        &Region));
    return 1;
};

static int lw32c_WriteRun(lua_State *L) {
    /* LuaWinCon.WriteRun(origin, {,attrib | string})
        Writes a series of attributes and strings. An attribute "sets" the current attribute. nil is a valid attribute,
    and causes attributes to not be written. A string will write out that string, wrapping at the edge of the console's
    screen. The empty string is a no-op. A nonnegative integer number will cause that many cells of attributes to be
    written. This function will not write past the end of the console screen's cellbuffer, nor will it scroll it.
        Returns a true if the operation was successful, false otherwise.
    */
    
    unsigned int num_args = lua_gettop(L);
    
    BOOL UsingAttributes  = FALSE;
    WORD CurrentAttribute = 0;
    
    LPCWSTR CurrentString;
    size_t  StringLength;
    
    HANDLE  ThisHandle = /*AA*/GetStdHandle(STD_OUTPUT_HANDLE)/*AB*/;
    COORD   ThisPoint;
    COORD   ThisSize;
    
    #define convert_to_UTF16(L, idx) \
        lua_pushcfunction(L, lw32c_internal_UTF8toUTF16); \
        lua_pushvalue(L, idx); \
        lua_call(L, 1, 1); \
        lua_copy(L, -1, idx); \
        lua_pop(L, 1)
    
    unsigned short sure_num_cells_to_write     = 0;
    unsigned short possible_num_cells_to_write = 0;
    
    DWORD dummy;
    
    /* Verify we have at least two arguments. */
    if (num_args < 2) {
        return luaL_error(L, "Insufficient arguments to WriteRun()!");
    };
    
    /* We'll be looping through our arguments twice. This first time verifies them, and also counts how many cells
    from beginning to end we will have to write to...we'll use this to determine the best way to perform our actual
    output. */
    if (!lw32c_internal_IsValidCoord(L, 1)) {
        return luaL_error(L, "Bad argument #1 to WriteRun(): not a 'coord'!");
    } else {
    int arg;
    for (arg = 2; arg <= num_args; arg++) {
        if        (lua_isnil(L, arg)) {
            UsingAttributes = FALSE;
        } else if (lw32c_internal_IsValidAttrib(L, arg)) {
            UsingAttributes = TRUE;
        } else if (lua_type(L, arg) == LUA_TSTRING) {
            if (lua_rawlen(L, arg) == 0) { /* Empty string is a no-op. */
                continue;
            };
            
            convert_to_UTF16(L, arg);
            possible_num_cells_to_write += lua_rawlen(L, arg) / sizeof(WCHAR);
            sure_num_cells_to_write = possible_num_cells_to_write;
        } else if (lua_type(L, arg) == LUA_TNUMBER) {
            int value;
            lua_Number fractional;
            lua_Number orig_value = lua_tonumber(L, arg);
            
            lua_pushvalue(L, arg);
            lua_pushinteger(L, 1);
            lua_arith(L, LUA_OPMOD);
            fractional = lua_tonumber(L, -1);
            
            /* Verify that it's sensible. */
            if (fractional != 0.0) {
                return luaL_error(L, "Bad argument #%d to WriteRun(): Non-integer number!", arg);
            } else if (orig_value < 0) {
                return luaL_error(L, "Bad argument #%d to WriteRun(): Negative number!", arg);
            };
            
            value = (int)orig_value;
            
            /* Overwrite the number on the stack with the version we will be really using anyway. */
            lua_pushinteger(L, value);
            lua_copy(L, -1, arg);
            
            lua_pop(L, 2);
            
            if ((UsingAttributes == TRUE) && (value > 0)) {
                possible_num_cells_to_write += value;
                sure_num_cells_to_write = possible_num_cells_to_write;
            } else { /* Encounter a number with no attribute currently set (or possibly, that number is 0) */
                possible_num_cells_to_write += value;
            };
        } else {
            luaL_error(L, "Bad argument #%d to WriteRun(): Not a nil, attrib, string, or number!", arg);
        };
    };};
    
    UsingAttributes = FALSE; /* Resetting it, just in case. */
    
    #undef convert_to_UTF16
        
    /* Okay, so now that all of our arguments have been verified, let's implement our command. */
    if (sure_num_cells_to_write == 0) {
        lua_pushboolean(L, 1);
    } else {
        CONSOLE_SCREEN_BUFFER_INFO ThisInfo;
        
        if (GetConsoleScreenBufferInfo(ThisHandle, &ThisInfo) == 0) {
            lua_pushboolean(L, 0);
        } else {
            unsigned int ThisOffset;
            #define within_bounds(point, size) ((point.X < size.X) && (point.Y < size.Y))
            ThisPoint = lw32c_internal_ToCOORD(L, 1, 0);
            ThisSize = ThisInfo.dwSize;
            
            ThisOffset = lw32c_internal_COORDtoOffset(ThisPoint, ThisSize);
            
            {int arg;
             for (arg = 2; arg <= num_args; arg++) {
                if (within_bounds(ThisPoint, ThisSize)) {
                    if        (lua_isnil(L, arg) == 1) {
                        UsingAttributes = FALSE;
                    } else if (lua_type(L, arg) == LUA_TSTRING) {
                        StringLength = lua_rawlen(L, arg);
                        if (StringLength > 0) {
                            StringLength = StringLength / sizeof(WCHAR);
                            CurrentString = (LPCWSTR)lua_tostring(L, arg);
                            if (UsingAttributes == TRUE) {
                                FillConsoleOutputAttribute(
                                    ThisHandle,
                                    CurrentAttribute,
                                    StringLength,
                                    ThisPoint,
                                    &dummy);
                            };
                            WriteConsoleOutputCharacterW(
                                ThisHandle,
                                CurrentString,
                                StringLength,
                                ThisPoint,
                                &dummy);
                            ThisOffset += StringLength;
                            ThisPoint = lw32c_internal_OffsetToCOORD(ThisOffset, ThisSize);
                        };
                    } else if (lua_type(L, arg) == LUA_TNUMBER) {
                        unsigned int value = lua_tounsigned(L, arg);
                        
                        if (value > 0) {
                            if (UsingAttributes == TRUE) {
                                FillConsoleOutputAttribute(
                                    ThisHandle,
                                    CurrentAttribute,
                                    value,
                                    ThisPoint,
                                    &dummy);
                            };
                            ThisOffset += value;
                            ThisPoint = lw32c_internal_OffsetToCOORD(ThisOffset, ThisSize);
                        };
                    } else { /* It could only be an attribute at this point. */
                        UsingAttributes = TRUE;
                        CurrentAttribute = lw32c_internal_AttribToWORD(L, arg);
                    };
                } else {
                    break;
                };
            };};
            #undef within_bounds
        };
        
        lua_pushboolean(L, 0);
    };

    return 1;
};

static int lw32c_SetCursorVisible(lua_State *L) {
    /* LuaWinCon.SetCussorVisible(bool)
    Sets whether the cursor is visible or not.
    Returns a true if the operation was successful, false otherwise.
    */
    
    HANDLE ThisHandle = /*AA*/GetStdHandle(STD_OUTPUT_HANDLE)/*AB*/;
    CONSOLE_CURSOR_INFO ThisInfo;
    
    /* Validate we have precisely one argument, and that it's a boolean. */
    {unsigned int num_args = lua_gettop(L);
    if        (num_args == 0) {
        return luaL_error(L, "Insufficient arguments to SetCursorVisible()!");
    } else if (num_args >  1) {
        return luaL_error(L, "Too many arguments to SetCursorVisible()!");
    } else if (lua_isboolean(L, 1) != 1) {
        return luaL_error(L, "Bad argument #1 to SetCursorVisible(): not a 'boolean'!");
    };};
    
    if (GetConsoleCursorInfo(ThisHandle, &ThisInfo) == 0) {
        lua_pushboolean(L, 0);
    } else {
        ThisInfo.bVisible = (lua_toboolean(L, 1) != 0) ? TRUE : FALSE;
        lua_pushboolean(L, SetConsoleCursorInfo(ThisHandle, &ThisInfo));
    };
    
    return 1;
};

static int lw32c_SetCursorPosition(lua_State *L) {
    /* LuaWinCon.SetCursorPosition(pos)
    Sets the cursor's position to 'pos', a 'coord' table.
    Returns a true if the operation was successful, false otherwise.
    */
    
    HANDLE ThisHandle = /*AA*/GetStdHandle(STD_OUTPUT_HANDLE)/*AB*/;
    COORD  ThisPos;
    
    /* Validate that we have precisely one argument, and that it's a 'coord'. */
    {unsigned int num_args = lua_gettop(L);
    if        (num_args == 0) {
        return luaL_error(L, "Insufficient arguments to SetCursorPosition()!");
    } else if (num_args >  1) {
        return luaL_error(L, "Too many arguments to SetCursorPosition()!");
    } else if (!lw32c_internal_IsValidCoord(L, 1)) {
        return luaL_error(L, "Bad argument #1 to SetCursorPosition(): not a 'coord'!");
    };};
    
    lua_pushinteger(L, 2);
    lua_gettable(L, 1);
    lua_pushinteger(L, 1);
    lua_gettable(L, 1);
    
    ThisPos.X = (SHORT)(lua_tounsigned(L, -1) - 1);
    ThisPos.Y = (SHORT)(lua_tounsigned(L, -2) - 1);
    
    lua_pushboolean(L, SetConsoleCursorPosition(ThisHandle, ThisPos));
    
    return 1;
};

static int lw32c_SetCursorSize(lua_State *L) {
    /* LuaWinCon.SetCursorSize(size)
    'size' must be a real number in the range 0.0 .. 1.0, specifying how much of a cell the cursor
    takes up.
    Returns a true if the operation was successful, false otherwise.
    */
    
    HANDLE ThisHandle = /*AA*/GetStdHandle(STD_OUTPUT_HANDLE)/*AB*/;
    CONSOLE_CURSOR_INFO ThisInfo;
    lua_Number ThisNumber;
    
    /* Validate we have precisely one argument, and that it's a number. */
    {unsigned int num_args = lua_gettop(L);
    if        (num_args == 0) {
        return luaL_error(L, "Insufficient arguments to SetCursorSize()!");
    } else if (num_args >  1) {
        return luaL_error(L, "Too many arguments to SetCursorSize()!");
    } else if (lua_isnumber(L, 1) != 1) {
        return luaL_error(L, "Bad argument #1 to SetCursorSize(): not a 'number'!");
    };};
    
    /* Now validate that the number is in the appropriate range. */
    ThisNumber = lua_tonumber(L, 1);
    if ((ThisNumber < 0.0) || (ThisNumber > 1.0)) {
        return luaL_error(L, "Bad argumnet #1 to SetCursorSize(): outside valid range!");
    };
    
    if (GetConsoleCursorInfo(ThisHandle, &ThisInfo) == 0) {
        lua_pushboolean(L, 0);
    } else {
        /* Now do our conversion. SetCursorSize() expects a number in range 0.0 .. 1.0, but
        CONSOLE_CURSOR_INFO.dwSize expects a number in range 1 .. 100. */
        ThisInfo.dwSize = (DWORD)((ThisNumber * 99) + 1);
        
        lua_pushboolean(L, SetConsoleCursorInfo(ThisHandle, &ThisInfo));
    };
    
    return 1;
};

static COORD lw32c_internal_ToCOORD(lua_State *L, int idx, int one_based) {
    /* This can return 0-based or 1-based COORDs, depending on if you are getting a point or a size. */
    COORD ThisCoord = {0, 0};
    
    if (lw32c_internal_IsValidCoord(L, idx)) {
        lua_pushinteger(L, 2);
        lua_gettable(L, idx);
        lua_pushinteger(L, 1);
        lua_gettable(L, idx);
        
        if (one_based == 0) {
            ThisCoord.X = (SHORT)lua_tounsigned(L, -1) - 1;
            ThisCoord.Y = (SHORT)lua_tounsigned(L, -2) - 1;
        } else {
            ThisCoord.X = (SHORT)lua_tounsigned(L, -1);
            ThisCoord.Y = (SHORT)lua_tounsigned(L, -2);
        };
        
        lua_pop(L, 2);
    };
    
    return ThisCoord;
};

static unsigned int lw32c_internal_COORDtoOffset(COORD point, COORD size) {
    /* Points are 0-based, sizes are 1-based. */
    return point.X + (point.Y * size.X);
};

static COORD lw32c_internal_OffsetToCOORD(unsigned int ofs, COORD size) {
    /* Returns a 0-based point, takes a 1-based size. */
    COORD ThisCoord = {ofs % size.X, ofs / size.X};
    return ThisCoord;
};

/* As far as I can tell, this function could very well be completely useless in the modern day.
It always reports that my mouse has 16 buttons, the maximum number that the API supports. In
reality, my mouse has 7, maybe 8 buttons. */
static int lw32c_GetNumberOfMouseButtons(lua_State *L) { /* FUNCTION PROBABLY TO BE DELETED */
    /* LuaWinCon.GetNumberOfMouseButtons()
    Returns the number of buttons whose state will be reported in mouse-input events, or nil
    if the data could not be retrieved.
    */
    
    DWORD NumOfButtons = 0;
    
    if (GetNumberOfConsoleMouseButtons(&NumOfButtons) == 0) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, (lua_Integer)NumOfButtons);
    };
    
    return 1;
};

/* This function has only a temporary life. */
static int lw32c_LowLevelMode(lua_State *L) { /* FUNCTION PROBABLY TO BE DELETED */
    HANDLE ThisHandle = /*AC*/GetStdHandle(STD_INPUT_HANDLE)/*AD*/;
    
    SetConsoleMode(ThisHandle, ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);
    FlushConsoleInputBuffer(ThisHandle);
    
    return 0;
};

static int lw32c_ClearInput(lua_State *L) {
    FlushConsoleInputBuffer(/*AC*/GetStdHandle(STD_INPUT_HANDLE)/*AD*/);
    
    return 0;
};

static int lw32c_ReadInput(lua_State *L) {
    INPUT_RECORD ThisRecord;
    HANDLE       ThisHandle = /*AC*/GetStdHandle(STD_INPUT_HANDLE)/*AD*/;
    DWORD        dummy;
    
    {BOOL Result;
     do {
        Result = ReadConsoleInputW(
                    ThisHandle,
                    &ThisRecord,
                    1,
                    &dummy);
    } while ((Result == 0) || ((ThisRecord.EventType != KEY_EVENT) && (ThisRecord.EventType != MOUSE_EVENT)));};
    
    if        (ThisRecord.EventType == KEY_EVENT) {
        lua_pushliteral(L, "key");
        lw32c_internal_PushKeyTable(L, &ThisRecord.Event.KeyEvent);
    } else if (ThisRecord.EventType == MOUSE_EVENT) {
        lua_pushliteral(L, "mouse");
        lw32c_internal_PushMouseTable(L, &ThisRecord.Event.MouseEvent);
    } else {
        luaL_error(L, "Something has gone terribly wrong in ReadInput()!");
    };

    return 2;
};

static int lw32c_PeekInput(lua_State *L) {
    INPUT_RECORD ThisRecord;
    HANDLE       ThisHandle = /*AC*/GetStdHandle(STD_INPUT_HANDLE)/*AD*/;
    DWORD        DidReadEvent;
    
    if ((PeekConsoleInputW(
            ThisHandle,
            &ThisRecord,
            1,
            &DidReadEvent) == 0) || (DidReadEvent == 0) || ((ThisRecord.EventType != KEY_EVENT) && (ThisRecord.EventType != MOUSE_EVENT))) {
        lua_pushnil(L);
        return 1;
    } else {
        if        (ThisRecord.EventType == KEY_EVENT) {
            lua_pushliteral(L, "key");
            lw32c_internal_PushKeyTable(L, &ThisRecord.Event.KeyEvent);
        } else if (ThisRecord.EventType == MOUSE_EVENT) {
            lua_pushliteral(L, "mouse");
            lw32c_internal_PushMouseTable(L, &ThisRecord.Event.MouseEvent);
        } else {
            luaL_error(L, "Something has gone terribly wrong in PeekInput()!");
        };
    };
    
    return 2;
};

static void lw32c_internal_PushKeyTable(lua_State *L, KEY_EVENT_RECORD* K) {
    char  UTF8Char[4] = {0,0,0,0};
    DWORD ControlKeys = K->dwControlKeyState;
    
    lua_createtable(L, 0, 5);
    
    if (K->bKeyDown) {
        lua_pushboolean(L, 1);
        lua_setfield(L, -2, "IsPressed");
    };
    
    lua_pushinteger(L, (lua_Integer)K->wRepeatCount);
    lua_setfield(L, -2, "RepeatCount");
    
    lua_pushinteger(L, (lua_Integer)K->wVirtualKeyCode);
    lua_setfield(L, -2, "KeyCode");
    
    lua_pushinteger(L, (lua_Integer)K->wVirtualScanCode);
    lua_setfield(L, -2, "ScanCode");
    
    lua_pushlstring(L, UTF8Char, lw32c_internal_WCHARtoUTF8(K->uChar.UnicodeChar, UTF8Char));
    lua_setfield(L, -2, "Character");
    
    #define CheckAndEmit(keystateconstant, fieldname) \
        if (ControlKeys & keystateconstant) { \
            lua_pushboolean(L, 1); \
            lua_setfield(L, -2, fieldname); \
        }
    #define CheckAndEmit2(keystateconstant, fieldname, fieldname2) \
        if (ControlKeys & keystateconstant) { \
            lua_pushboolean(L, 1); \
            lua_setfield(L, -2, fieldname); \
            lua_pushboolean(L, 1); \
            lua_setfield(L, -2, fieldname2); \
        }
    
    CheckAndEmit(CAPSLOCK_ON,        "CapsLockIsOn");
    CheckAndEmit(NUMLOCK_ON,         "NumLockIsOn");
    CheckAndEmit(SCROLLLOCK_ON,      "ScrollLockIsOn");
    
    CheckAndEmit(ENHANCED_KEY,       "KeyIsEnhanced");
    
    CheckAndEmit(SHIFT_PRESSED,      "ShiftIsPressed");
    CheckAndEmit(LEFT_ALT_PRESSED,   "LeftAltIsPressed");
    CheckAndEmit(LEFT_CTRL_PRESSED,  "LeftCtrlIsPressed");
    CheckAndEmit2(RIGHT_ALT_PRESSED, "RightAltIsPressed", "AltGrIsPressed");
    CheckAndEmit(RIGHT_CTRL_PRESSED, "RightCtrlIsPressed");
    
    #undef CheckAndEmit
    #undef CheckAndEmit2

    return;
};

static int lw32c_internal_WCHARtoUTF8(WCHAR InChar, char* OutChar) {
    /* Encodes a given UTF-16 Code Unit in UTF-8, returns the number of bytes the encoding uses. 
    Resulting string is NOT NUL-terminated. As a special case, NUL is instead encoded as an empty string. */
    
    if        (InChar == 0) {
        return 0;
    } else if (InChar < 0x00FF) {
        OutChar[0] = (char)InChar;
        return 1;
    } else if (InChar < 0x0FFF) {
        /* Two-byte encoding */
        OutChar[0] = 0xC0 | ((InChar >> 6 ) & 0x1F);
        OutChar[1] = 0x80 | ( InChar        & 0x3F);
        return 2;
    } else {
        /* Three-byte encoding */
        OutChar[0] = 0xE0 | ((InChar >> 12) & 0x0F);
        OutChar[1] = 0x80 | ((InChar >> 6 ) & 0x3F);
        OutChar[2] = 0x80 | ( InChar        & 0x3F);
        return 3;
    };
};

static void lw32c_internal_PushMouseTable(lua_State *L, MOUSE_EVENT_RECORD* M) {
    
    lua_createtable(L, 0, 7);
    
    lua_createtable(L, 2, 0);
    lua_pushinteger(L, (lua_Integer)(M->dwMousePosition.X + 1));
    lua_pushinteger(L, (lua_Integer)(M->dwMousePosition.Y + 1));
    lua_rawseti(L, -3, 2);
    lua_rawseti(L, -2, 1);
    lua_setfield(L, -2, "Position");
    
    lua_createtable(L, 16, 0);
    {int i;
     DWORD mask = 1;
     for (i = 1; i <= 16; i++) {
        lua_pushboolean(L, M->dwButtonState & mask);
        lua_rawseti(L, -2, i);
        mask = mask << 1;
    };};
    lua_setfield(L, -2, "Buttons");
    
    lua_pushboolean(L, (M->dwEventFlags == 0));
    lua_setfield(L, -2, "Buttoned");
    
    lua_pushboolean(L, M->dwEventFlags & DOUBLE_CLICK);
    lua_setfield(L, -2, "DoubleClicked");
    
    lua_pushboolean(L, M->dwEventFlags & MOUSE_MOVED);
    lua_setfield(L, -2, "Moved");
    
    lua_pushboolean(L, M->dwEventFlags & MOUSE_WHEELED);
    lua_setfield(L, -2, "Wheeled");
    
    lua_pushboolean(L, M->dwEventFlags & MOUSE_HWHEELED);
    lua_setfield(L, -2, "HWheeled");
    
    return;
};

#if 0
static int lw32c_Beep1(lua_State *L) {
    /* Just a test function, don't mind me. */
    DWORD FrequencyInHertz;
    DWORD DurationInMilliseconds;
    
    FrequencyInHertz = lua_tounsigned(L, 1);
    DurationInMilliseconds = (DWORD)(lua_tonumber(L, 2) * 1000); /* This parameter in Lua, is in seconds, not milliseconds. */
    
    lua_pushboolean(L, Beep(FrequencyInHertz, DurationInMilliseconds));

    return 1;
};

static int lw32c_Beep2(lua_State *L) {
    /* Just a test function, don't mind me. */
    MessageBeep(0xFFFFFFFF);
    
    return 0;
};

static int lw32c_Beep3(lua_State *L) {
    /* Just a test function, don't mind me. */
    FlashWindow(GetConsoleWindow(), TRUE);

    return 0;
};
#endif

#if 0
static int lw32c_GetClipboardText(lua_State *L) {
    /* Test version, will be put in another library. */
    /* Returns text if it succeeds, nil if it fails. Could fail due to not having text or due
    to being unable to acquire the clipboard. */
    
    if (OpenClipboard(GetConsoleWindow())) {
        if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
            HANDLE GlobalHandle = GetClipboardData(CF_UNICODETEXT);
            if (GlobalHandle != NULL) {
                LPCWSTR ThisString = GlobalLock(GlobalHandle);
                if (ThisString != NULL) {
                    luaW_pushstring(L, ThisString);
                    GlobalUnlock(GlobalHandle);
                } else { lua_pushnil(L); };
            } else { lua_pushnil(L); };
            CloseClipboard();
        } else { lua_pushnil(L); };
    } else { lua_pushnil(L); };
    
    return 1;
};

static int lw32c_SetClipboardText(lua_State *L) {
    /* Test version, will be put in another library. */
    /* Returns true if the function succeeded and false if it failed. */
    const WCHAR ThisNul = 0;
    
    if (OpenClipboard(GetConsoleWindow())) {
        if (EmptyClipboard()) {
            /*Lua only guarantees an ASCII NUL at the end, we need a UTF-16 NUL. */
            lua_pushcfunction(L, lw32c_internal_UTF8toUTF16);
            lua_pushvalue(L, 1);
            lua_call(L, 1, 1);
            lua_pushlstring(L, (const char *)&ThisNul, sizeof(WCHAR));
            lua_concat(L, 2);
            
            lua_pushboolean(L, (SetClipboardData(CF_UNICODETEXT, (HANDLE)lua_tostring(L, -1)) == NULL));
        } else { lua_pushboolean(L, 0); };
    } else { lua_pushboolean(L, 0); };
    
    return 1;
};
#endif

const luaL_Reg FuncsToReg[] = {
    {"GetLargestConsoleSize",   lw32c_GetLargestConsoleSize},
    {"SetConsoleSize",          lw32c_SetConsoleSize},
    {"SetTitle",                lw32c_SetTitle},
/*  {"GetColorTable",           lw32c_GetColorTable}, */
    {"FillRegion",              lw32c_FillRegion},
    {"WriteRegion",             lw32c_WriteRegion},
    {"WriteRun",                lw32c_WriteRun},
    {"SetCursorVisible",        lw32c_SetCursorVisible},
    {"SetCursorPosition",       lw32c_SetCursorPosition},
    {"SetCursorSize",           lw32c_SetCursorSize},
    {"GetNumberOfMouseButtons", lw32c_GetNumberOfMouseButtons},
    {"LowLevelMode",            lw32c_LowLevelMode},
    {"ClearInput",              lw32c_ClearInput},
    {"ReadInput",               lw32c_ReadInput},
    {"PeekInput",               lw32c_PeekInput},
/*  {"Beep1",                   lw32c_Beep1}, */
/*  {"Beep2",                   lw32c_Beep2}, */
/*  {"Beep3",                   lw32c_Beep3}, */
/*  {"GetClipboardText",        lw32c_GetClipboardText}, */
/*  {"SetClipboardText",        lw32c_SetClipboardText}, */
    {NULL,                      NULL}
    };

int luaopen_LuaWinCon(lua_State *L) {
    lua_newtable(L);
    luaL_setfuncs(L, FuncsToReg, 0); /* Later, we might want to use upvalues for giving out the HANDLE */
    return 1;
};