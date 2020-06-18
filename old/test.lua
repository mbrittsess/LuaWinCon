wincon = require "LuaWinCon"
wincon.SetConsoleSize({140,50})
wincon.SetTitle("LuaWinCon Test Script")
for i = 1,16 do
    for j = 1,16 do
        wincon.FillRegion({((i-1)*5)+1,((j-1)*3)+1},{5,3},{i,j},"#")
    end
end