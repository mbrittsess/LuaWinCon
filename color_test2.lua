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