local lwc = require "LuaWinCon"

local defattr1 = lwc.GetDefaultAttrib()

lwc.NewConsole()
local defattr2 = lwc.GetDefaultAttrib()

io.stderr:write(string.format("Initial Default Attribute: {%02u, %02u}\n", defattr1[1], defattr1[2]))
io.stderr:write(string.format("New Default Attribyte:     {%02u, %02u}\n", defattr2[1], defattr2[2]))
os.exit()