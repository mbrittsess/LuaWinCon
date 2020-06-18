local lwc = require "LuaWinCon"
lwc.ClearInput()
lwc.LowLevelMode()
local Default = lwc.DefaultBuffer()
Default:SetWindowPositionAndSize({1,1},{80,25})
Default:SetScreenBufferSize({80,25})
Default:FillRegion({1,1},{80,25},' ')
Default:SetCursorVisible(false)
Default:WriteRun({1,1},"This is the default buffer!")
local Buffers = {
    ["1"] = lwc.GetNewBuffer(),
    ["2"] = lwc.GetNewBuffer(),
    ["3"] = lwc.GetNewBuffer(),
    ["4"] = lwc.GetNewBuffer(),
    ["5"] = lwc.GetNewBuffer()
}
for name,Buffer in pairs(Buffers) do
    local size = {50,tonumber(name)*2}
    Buffer:SetWindowPositionAndSize({1,1}, size)
    Buffer:SetScreenBufferSize(size)
    Buffer:FillRegion({1,1}, size, ' ', {16-tonumber(name),tonumber(name)})
    Buffer:WriteRun({1,1}, "This is buffer #"..name.."!")
    Buffer:SetCursorPosition({tonumber(name),1})
    Buffer:SetCursorVisible(true)
end
while true do
    local typeof, record = lwc.FetchInput()
    if (typeof == "mouse") then
        if (record.Action == "doubleclicked") then
            Default:SetCursorVisible(true)
            Default:SetScreenBufferSize({140,300})
            Default:SetWindowPositionAndSize({1,1},{140,50})
            Default:SetCursorPosition({1,1})
            Default:FillRegion({1,1},{140,300},' ')
            Default:MakeActive()
            os.exit()
        end
    else
        if Buffers[record.Key] then Buffers[record.Key]:MakeActive() end
        Default:WriteRun({1,2},"Pressed key "..record.Key)
    end
end