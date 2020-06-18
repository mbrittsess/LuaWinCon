print = function(...)
    local res = {}
    for i,v in ipairs{...} do
        res[i] = tostring(v)
    end
    res = table.concat(res, '\t') .. '\n'
    io.stderr:write(res)
end

local HSLtoRGB
do refs = {
    [0] = "cxz";
    [1] = "xcz";
    [2] = "zcx";
    [3] = "zxc";
    [4] = "xzc";
    [5] = "czx";
}
HSLtoRGB = function(Hue, Saturation, Lightness)
    --Values for HSL are expected to be in the ranges 0..360, 0..1, and 0..1, respectively.
    --Returns an {r,g,b} value where each channel is an integer in the range 0..255
    local val = {c=nil, x=nil, z=0}
    val.c = (1 - math.abs((2*Lightness) - 1)) * Saturation
    local H = (Hue / 60) % 6
    val.x = val.c * (1 - math.abs((H%2)-1))
    local rgb = {}
    local refstr = refs[math.floor(H)]
    for pos, char in string.gmatch("rgb", "()(.)") do
        rgb[char] = val[refstr:sub(pos,pos)]
    end
    local m = Lightness - (val.c / 2)
    if m < 0 then m = 0 end
    for k,v in pairs(rgb) do rgb[k] = (v + m) end
    return rgb
end end

local function PrintColTable(coltbl)
    for i,v in ipairs(coltbl) do
        print(string.format("Color Table Entry #%d: (%d %d %d)", i, v.r, v.g, v.b))
    end
    print("\n")
end

local colortbl = {}
local dcolortbl = {}
for i = 1,16 do colortbl[i] = HSLtoRGB(240,1,i/17) end
for i = 1,8 do 
    dcolortbl[i] = HSLtoRGB(((i/8)*360)%360,1,0.25)
    dcolortbl[i+8] = HSLtoRGB(((i/8)*360)%360,1,0.5)
    print(string.format("Values passed to HSLtoRGB for [%d] and [%d]: %f %f %f/%f", i, i+8, ((i/8)*360)%360,1,0.5,1))
end

local lwc = require "LuaWinCon"
--[[
print("Original color table of default buffer before creation of new console:")
PrintColTable(lwc.DefaultBuffer():GetColorTable())
--]]
lwc.NewConsole()
lwc.ClearInput()
lwc.LowLevelMode()
local default = lwc.DefaultBuffer()
local alternate = lwc.GetNewBuffer()
local current = default

for i,rgb in ipairs(colortbl) do
    io.stderr:write(string.format("Entry %-2d: (%5.3f %5.3f %5.3f)\n", i, rgb.r, rgb.g, rgb.b))
end
for i,rgb in ipairs(dcolortbl) do
    io.stderr:write(string.format("Entry %-2d: (%5.3f %5.3f %5.3f)\n", i, rgb.r, rgb.g, rgb.b))
end

--[[
print("Original Color Table for Default Buffer:")
PrintColTable(default:GetColorTable())
print("Original Color Table for Alternate Buffer:")
PrintColTable(alternate:GetColorTable())
--]]

print("Result of setting color table: ", alternate:SetColorTable(colortbl))
--[[
print("Color Table of Default Buffer right after setting color table for Alternate Buffer:")
PrintColTable(default:GetColorTable())
print("Color Table of Alternate Buffer right after setting color table for Alternate Buffer:")
PrintColTable(alternate:GetColorTable())
--]]
print("Result of setting default color table: ", default:SetColorTable(dcolortbl))
--[[
print("Color Table of Default Buffer right after setting color table for Default Buffer:")
PrintColTable(default:GetColorTable())
print("Color Table of Alternate Buffer right after setting color table for Default Buffer:")
PrintColTable(default:GetColorTable())
--]]

local drawargs = {}
for i = 1,31,2 do drawargs[i] = {16,math.ceil(i/2)} drawargs[i+1] = 1 end

for _,buf in ipairs{default,alternate} do
    buf:SetWindowPositionAndSize({1,1},{32,20})
    buf:SetScreenBufferSize({32,20})
    buf:WriteRun({1,2},table.unpack(drawargs))
end

default:MakeActive()

local colors = {[default] = dcolortbl, [alternate] = colortbl}

while true do
    local typeof, record = lwc.FetchInput()
    if typeof == "mouse" then
        if record.Action == "doubleclicked" then
            os.exit()
        elseif record.Action == "buttoned" then
            current = (current == default) and alternate or default
            print("Attempt to change active SB: ", current:MakeActive())
            print("Attempt to change colortbl: ", current:SetColorTable(colors[current]))
        end
    end
end