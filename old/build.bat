@echo off
setlocal
::If you are not Matthew Sessions, or you are but compiling on Lionel instead of Martin, you'll need to adjust these environment variables:
set SDK="C:\devtools\WindowskSDK"
set DDK="C:\devtools\WindowsDDK"
set LuaP="C:\Program Files (x86)\Lua\5.2"

set CL86="C:\devtools\WindowsDDK\bin\x86\x86\cl.exe"
set LINK86="C:\devtools\WindowsDDK\bin\x86\x86\link.exe"

%CL86% /c /I%SDK%\Include /I%DDK%\inc\crt /I%DDK%\inc\api /I%LuaP%\include /TcLuaWinCon.c /FoLuaWinCon.obj
if not errorlevel 1 (
:: Remember to finish up the cmdline for this thing here...
%LINK86% /DLL /EXPORT:luaopen_LuaWinCon /LIBPATH:%DDK%\lib\win7\i386 /LIBPATH:%DDK%\lib\Crt\i386 /LIBPATH:%LuaP%\lib /LIBPATH:%SDK%\Lib /DEFAULTLIB:lua52.lib /DEFAULTLIB:User32.lib LuaWinCon.obj /OUT:LuaWinCon.dll
)

del LuaWinCon.obj
del LuaWinCon.exp
del LuaWinCon.lib
endlocal