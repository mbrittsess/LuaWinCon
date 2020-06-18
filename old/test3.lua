local wincon = require "LuaWinCon"

wincon.SetTitle("LuaWinCon Test Script #3")
wincon.SetConsoleSize({80,25})
wincon.FillRegion({1,1},{80,25},{16,1},' ')
wincon.SetCursorVisible(true)
wincon.SetCursorSize(1.0)

wincon.LowLevelMode()
wincon.ClearInput()

wincon.WriteRun({1,1},"Most recent:")
wincon.WriteRun({4,2},"Mouse position:")
wincon.WriteRun({4,3},"Character:")
wincon.WriteRun({4,4},"Code Point:")
wincon.WriteRun({4,5},"Key Code:")
wincon.WriteRun({4,6},"Scan Code:")
wincon.WriteRun({4,7},"MouseButton:")

local exit_func = function()
    wincon.SetConsoleSize(140,50)
    wincon.FillRegion({1,1},{140,50},{16,1},' ')
    wincon.SetCursorVisible(true)
    os.exit(true)
end

local key_defs
local func_tbl = {
    ["key"] = function(tbl)
        if tbl.IsPressed then
            wincon.WriteRun({20,3}, string.format("%-4s", tbl.Character))
            local bytes = {string.byte(tbl.Character, 1, 3)}
            for i = 1,3 do
             wincon.WriteRun({20+((i-1)*4),4}, string.format("%03u", bytes[i] or 0))
            end
            wincon.WriteRun({20,5}, string.format("%-20s", key_defs[tbl.KeyCode] or 'undefined'))
            wincon.WriteRun({20,6}, string.format("%-6u", tbl.ScanCode))
        end
    end;
    ["mouse"] = function(tbl)
        wincon.WriteRun({20,2},string.format("%02u %02u", tbl.Position[1], tbl.Position[2]))
        if tbl.Buttoned then
            local this = ""
            for k,v in pairs(tbl.Buttons) do if v then this = this .. tostring(k) .. ' ' end end
            wincon.WriteRun({20,7}, this)
        end
        if tbl.Moved then wincon.SetCursorPosition(tbl.Position) end
        if tbl.DoubleClicked then exit_func() end
    end;
}

key_defs = {
    [0x01] = 'lbutton',
    [0x02] = 'rbutton',
    [0x03] = 'cancel',
    [0x04] = 'mbutton',
    [0x05] = 'xbutton1',
    [0x06] = 'xbutton2',
    [0x08] = 'back',
    [0x09] = 'tab',
    [0x0C] = 'clear',
    [0x0D] = 'return',
    [0x10] = 'shift',
    [0x11] = 'control',
    [0x12] = 'menu',
    [0x13] = 'pause',
    [0x14] = 'capital',
    [0x15] = 'kana',
    [0x15] = 'hangeul',
    [0x15] = 'hangul',
    [0x17] = 'junja',
    [0x18] = 'final',
    [0x19] = 'hanja',
    [0x19] = 'kanji',
    [0x1B] = 'escape',
    [0x1C] = 'convert',
    [0x1D] = 'nonconvert',
    [0x1E] = 'accept',
    [0x1F] = 'modechange',
    [0x20] = 'space',
    [0x21] = 'prior',
    [0x22] = 'next',
    [0x23] = 'end',
    [0x24] = 'home',
    [0x25] = 'left',
    [0x26] = 'up',
    [0x27] = 'right',
    [0x28] = 'down',
    [0x29] = 'select',
    [0x2A] = 'print',
    [0x2B] = 'execute',
    [0x2C] = 'snapshot',
    [0x2D] = 'insert',
    [0x2E] = 'delete',
    [0x2F] = 'help',
    [0x5B] = 'lwin',
    [0x5C] = 'rwin',
    [0x5D] = 'apps',
    [0x5F] = 'sleep',
    [0x60] = 'numpad0',
    [0x61] = 'numpad1',
    [0x62] = 'numpad2',
    [0x63] = 'numpad3',
    [0x64] = 'numpad4',
    [0x65] = 'numpad5',
    [0x66] = 'numpad6',
    [0x67] = 'numpad7',
    [0x68] = 'numpad8',
    [0x69] = 'numpad9',
    [0x6A] = 'multiply',
    [0x6B] = 'add',
    [0x6C] = 'separator',
    [0x6D] = 'subtract',
    [0x6E] = 'decimal',
    [0x6F] = 'divide',
    [0x70] = 'f1',
    [0x71] = 'f2',
    [0x72] = 'f3',
    [0x73] = 'f4',
    [0x74] = 'f5',
    [0x75] = 'f6',
    [0x76] = 'f7',
    [0x77] = 'f8',
    [0x78] = 'f9',
    [0x79] = 'f10',
    [0x7A] = 'f11',
    [0x7B] = 'f12',
    [0x7C] = 'f13',
    [0x7D] = 'f14',
    [0x7E] = 'f15',
    [0x7F] = 'f16',
    [0x80] = 'f17',
    [0x81] = 'f18',
    [0x82] = 'f19',
    [0x83] = 'f20',
    [0x84] = 'f21',
    [0x85] = 'f22',
    [0x86] = 'f23',
    [0x87] = 'f24',
    [0x90] = 'numlock',
    [0x91] = 'scroll',
    [0x92] = 'oem_nec_equal',
    [0x92] = 'oem_fj_jisho',
    [0x93] = 'oem_fj_masshou',
    [0x94] = 'oem_fj_touroku',
    [0x95] = 'oem_fj_loya',
    [0x96] = 'oem_fj_roya',
    [0xA0] = 'lshift',
    [0xA1] = 'rshift',
    [0xA2] = 'lcontrol',
    [0xA3] = 'rcontrol',
    [0xA4] = 'lmenu',
    [0xA5] = 'rmenu',
    [0xA6] = 'browser_back',
    [0xA7] = 'browser_forward',
    [0xA8] = 'browser_refresh',
    [0xA9] = 'browser_stop',
    [0xAA] = 'browser_search',
    [0xAB] = 'browser_favorites',
    [0xAC] = 'browser_home',
    [0xAD] = 'volume_mute',
    [0xAE] = 'volume_down',
    [0xAF] = 'volume_up',
    [0xB0] = 'media_next_track',
    [0xB1] = 'media_prev_track',
    [0xB2] = 'media_stop',
    [0xB3] = 'media_play_pause',
    [0xB4] = 'launch_mail',
    [0xB5] = 'launch_media_select',
    [0xB6] = 'launch_app1',
    [0xB7] = 'launch_app2',
    [0xBA] = 'oem_1',
    [0xBB] = 'oem_plus',
    [0xBC] = 'oem_comma',
    [0xBD] = 'oem_minus',
    [0xBE] = 'oem_period',
    [0xBF] = 'oem_2',
    [0xC0] = 'oem_3',
    [0xDB] = 'oem_4',
    [0xDC] = 'oem_5',
    [0xDD] = 'oem_6',
    [0xDE] = 'oem_7',
    [0xDF] = 'oem_8',
    [0xE1] = 'oem_ax',
    [0xE2] = 'oem_102',
    [0xE3] = 'ico_help',
    [0xE4] = 'ico_00',
    [0xE5] = 'processkey',
    [0xE6] = 'ico_clear',
    [0xE7] = 'packet',
    [0xE9] = 'oem_reset',
    [0xEA] = 'oem_jump',
    [0xEB] = 'oem_pa1',
    [0xEC] = 'oem_pa2',
    [0xED] = 'oem_pa3',
    [0xEE] = 'oem_wsctrl',
    [0xEF] = 'oem_cusel',
    [0xF0] = 'oem_attn',
    [0xF1] = 'oem_finish',
    [0xF2] = 'oem_copy',
    [0xF3] = 'oem_auto',
    [0xF4] = 'oem_enlw',
    [0xF5] = 'oem_backtab',
    [0xF6] = 'attn',
    [0xF7] = 'crsel',
    [0xF8] = 'exsel',
    [0xF9] = 'ereof',
    [0xFA] = 'play',
    [0xFB] = 'zoom',
    [0xFC] = 'noname',
    [0xFD] = 'pa1',
    [0xFE] = 'oem_clear',
    [  48] = '\048',
    [  49] = '\049',
    [  50] = '\050',
    [  51] = '\051',
    [  52] = '\052',
    [  53] = '\053',
    [  54] = '\054',
    [  55] = '\055',
    [  56] = '\056',
    [  57] = '\057',
    [  65] = '\065',
    [  66] = '\066',
    [  67] = '\067',
    [  68] = '\068',
    [  69] = '\069',
    [  70] = '\070',
    [  71] = '\071',
    [  72] = '\072',
    [  73] = '\073',
    [  74] = '\074',
    [  75] = '\075',
    [  76] = '\076',
    [  77] = '\077',
    [  78] = '\078',
    [  79] = '\079',
    [  80] = '\080',
    [  81] = '\081',
    [  82] = '\082',
    [  83] = '\083',
    [  84] = '\084',
    [  85] = '\085',
    [  86] = '\086',
    [  87] = '\087',
    [  88] = '\088',
    [  89] = '\089',
    [  90] = '\090',
}

while true do
    local event_type, event_tbl = wincon.ReadInput()
    func_tbl[event_type](event_tbl)
end
