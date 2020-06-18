--[[This chunk will return the metatable used for coords in the basic Win32 implementation of the LuaWinCon library. It is compatible with
both Lua 5.1 and Lua 5.2]]
local meta = {}
local method = {}

--We want the functions here to run as fast as possible, localize EVERYTHING
local tonumber, setmetatable, floor, fmt = tonumber, setmetatable, math.floor, string.format

local function IsACoord(cord)
    return tonumber(cord[1]) and tonumber(cord[2])
end

local function MakeCoord(cord)
    return setmetatable(cord, meta)
end

function meta.__add(op1, op2)
    if not IsACoord(op1) and IsACoord(op2) then
        error("Tried to add a non-coord to a coord!", 2)
    else
        return MakeCoord{floor(op1[1]+op2[1]), floor(op1[2]+op2[2])}
    end
end

function meta.__mul(op1, op2) --I feel that this could be better-written.
    local cord, num = nil, nil
    if IsACoord(op1) then
        if IsACoord(op2) then
            error("Tried to multiply a coord by a coord!", 2)
        elseif not tonumber(op2) then
            error("Tried to multiply a coord by a non-number!")
        else
            cord, num = op1, op2
        end
    else --IsACoord(op2) *must* be true in this case
        if not tonumber(op1) then
            error("Tried to multiply a coord by a non-number!")
        else
            cord, num = op2, op1
        end
    end
    
    return MakeCoord{floor(cord[1]*num), floor(cord[2]*num)}
end

do local map_table = {
    ["x"] = 1,
    ["w"] = 1,
    ["y"] = 2,
    ["h"] = 2
}

function meta.__index(cord, key)
    local mapped = map_table[key]
    return mapped and cord[mapped] or method[key]
end

function meta.__newindex(cord, key, val)
    local mapped = map_table[key]
    if mapped then cord[mapped] = val else cord[key] = val end
end 

end

function meta.__tostring(cord)
    return fmt("(%d, %d)", cord[1], cord[2])
end

return meta