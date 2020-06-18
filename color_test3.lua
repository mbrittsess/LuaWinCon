local function LogPrint(...)
    local res = {}
    for i,v in ipairs{...} do
        res[i] = tostring(v)
    end
    res = table.concat(res, '\t') .. '\n'
    io.stderr:write(res)
end

print = LogPrint

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

local function LogColorTbl(coltbl)
     for i,v in ipairs(coltbl) do
        LogPrint(string.format("Color Table Entry #%d: (%f %f %f)", i, v.r, v.g, v.b))
    end
    LogPrint("")
end

local function GenShades(Hue)
    local coltbl = {}
    for i = 1,16 do
        coltbl[i] = HSLtoRGB(Hue, 1, i/17)
    end
    LogColorTbl(coltbl)
    return coltbl
end


local lwc = require "LuaWinCon"
lwc.NewConsole()
lwc.ClearInput()
lwc.LowLevelMode()

local drawargs = {}
for i = 1,31,2 do drawargs[i] = {16,math.ceil(i/2)} drawargs[i+1] = 1 end

local Buf1 = lwc.DefaultBuffer()
lwc.SetDefaultColorTable(GenShades(90))
local Buf2 = lwc.GetNewBuffer()
local Buf3 = lwc.GetNewBuffer()
local all = {Buf1, Buf2, Buf3}

for _,buf in ipairs(all) do
    buf:SetWindowPositionAndSize({1,1},{80,25})
    buf:SetScreenBufferSize({80,25})
    buf:WriteRun({1,2},table.unpack(drawargs))
    buf:WriteRun({1,1},"1".._)
end

local stepstotake = {
    function()
        Buf2:SetColorTable(GenShades(120))
        for _,buf in ipairs(all) do buf:WriteRun({1,1},"2") end
    end;
    function()
        Buf2:MakeActive()
        for _,buf in ipairs(all) do buf:WriteRun({1,1},"3") end
    end;
    function()
        Buf3:MakeActive()
        for _,buf in ipairs(all) do buf:WriteRun({1,1},"3.3") end
    end;
    function()
        Buf2:MakeActive()
        for _,buf in ipairs(all) do buf:WriteRun({1,1},"3.6") end
    end;
    function()
        Buf2:SetColorTable(GenShades(240))
        for _,buf in ipairs(all) do buf:WriteRun({1,1},"4") end
    end;
    function()
        Buf3:SetColorTable(GenShades(30))
        for _,buf in ipairs(all) do buf:WriteRun({1,1},"5") end
    end;
    function()
        Buf1:MakeActive()
        for _,buf in ipairs(all) do buf:WriteRun({1,1},"5.5") end
    end;
    function()
        Buf3:MakeActive()
        for _,buf in ipairs(all) do buf:WriteRun({1,1},"6  ") end
    end;
    function()
        Buf1:MakeActive()
        for _,buf in ipairs(all) do buf:WriteRun({1,1},"7") end
    end;
    function()
        Buf2:MakeActive()
        for _,buf in ipairs(all) do buf:WriteRun({1,1},"8") end
    end;
    function()
        os.exit()
    end;
}

do local counter = 1
while true do
    local typeof, record = lwc.FetchInput()
    if (typeof == "mouse") and (record.Action == "doubleclicked") then
        stepstotake[counter]()
        counter = counter +1
    end
end end