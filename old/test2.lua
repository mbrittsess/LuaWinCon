wincon = require "LuaWinCon"
wincon.SetConsoleSize({140,50})
wincon.SetTitle("LuaWinCon Test Script #2")
wincon.WriteRegion({3,10},{10,4},nil,"Default. ",{10,14},"Psychedelic. ",nil,3,"Default again.")