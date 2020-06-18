local lwc = require "LuaWinCon"
lwc.NewConsole()
local sb = lwc.DefaultBuffer()
sb:SetWindowPositionAndSize({1,1},{1,1})
sb:SetScreenBufferSize({80,25})
sb:SetWindowPositionAndSize({1,1},{80,25})
sb:FillRegion({1,1},{80,20},' ')
lwc.ClearInput()
lwc.LowLevelMode()
while true do
    local typeof, record = lwc.FetchInput()
    local downtext, keycode, scancode, keyname = "", "", "", ""
    local mousepos = {0,0}
    if typeof == "key" then
        downtext = record.IsDown and "Down" or "Up"
        keycode = tostring(record.KeyCode)
        scancode = tostring(record.ScanCode)
        keyname = record.Key
    elseif typeof == "mouse" then
        mousepos = record.Position
    end
    downtext = downtext .. string.rep(' ', 4 - #downtext)
    keycode  = keycode  .. string.rep(' ', 8 - #keycode)
    scancode = scancode .. string.rep(' ', 8 - #scancode)
    keyname  = keyname  .. string.rep(' ', 8 - #keyname)
    local mouse1 = tostring(mousepos[1])
    local mouse2 = tostring(mousepos[2])
    
    mouse1 = mouse1 .. string.rep(' ', 3 - #mouse1)
    mouse2 = mouse2 .. string.rep(' ', 3 - #mouse2)
    
    sb:WriteRun({1,1}, downtext)
    sb:WriteRun({1,2}, keycode)
    sb:WriteRun({1,3}, scancode)
    sb:WriteRun({1,4}, keyname)
    sb:WriteRun({1,5}, mouse1, ' ', mouse2)
    sb:WriteRun({1,6}, typeof)
    
    if (typeof == "mouse") and (record.Action == "doubleclicked") then os.exit() end
end