const char* vk_MakeTableFuncString = (
"return {\n"
"    [0x01] = 'lbutton',\n"
"    [0x02] = 'rbutton',\n"
"    [0x03] = 'cancel',\n"
"    [0x04] = 'mbutton',\n"
"    [0x05] = 'xbutton1',\n"
"    [0x06] = 'xbutton2',\n"
"    [0x08] = 'back',\n"
"    [0x09] = 'tab',\n"
"    [0x0C] = 'clear',\n"
"    [0x0D] = 'return',\n"
"    [0x10] = 'shift',\n"
"    [0x11] = 'control',\n"
"    [0x12] = 'menu',\n"
"    [0x13] = 'pause',\n"
"    [0x14] = 'capital',\n"
"    [0x15] = 'kana',\n"
"    [0x17] = 'junja',\n"
"    [0x18] = 'final',\n"
"    [0x19] = 'hanja',\n"
"    [0x1B] = 'escape',\n"
"    [0x1C] = 'convert',\n"
"    [0x1D] = 'nonconvert',\n"
"    [0x1E] = 'accept',\n"
"    [0x1F] = 'modechange',\n"
"    [0x20] = 'space',\n"
"    [0x21] = 'prior',\n"
"    [0x22] = 'next',\n"
"    [0x23] = 'end',\n"
"    [0x24] = 'home',\n"
"    [0x25] = 'left',\n"
"    [0x26] = 'up',\n"
"    [0x27] = 'right',\n"
"    [0x28] = 'down',\n"
"    [0x29] = 'select',\n"
"    [0x2A] = 'print',\n"
"    [0x2B] = 'execute',\n"
"    [0x2C] = 'snapshot',\n"
"    [0x2D] = 'insert',\n"
"    [0x2E] = 'delete',\n"
"    [0x2F] = 'help',\n"
"    [0x5B] = 'lwin',\n"
"    [0x5C] = 'rwin',\n"
"    [0x5D] = 'apps',\n"
"    [0x5F] = 'sleep',\n"
"    [0x60] = 'numpad0',\n"
"    [0x61] = 'numpad1',\n"
"    [0x62] = 'numpad2',\n"
"    [0x63] = 'numpad3',\n"
"    [0x64] = 'numpad4',\n"
"    [0x65] = 'numpad5',\n"
"    [0x66] = 'numpad6',\n"
"    [0x67] = 'numpad7',\n"
"    [0x68] = 'numpad8',\n"
"    [0x69] = 'numpad9',\n"
"    [0x6A] = 'multiply',\n"
"    [0x6B] = 'add',\n"
"    [0x6C] = 'separator',\n"
"    [0x6D] = 'subtract',\n"
"    [0x6E] = 'decimal',\n"
"    [0x6F] = 'divide',\n"
"    [0x70] = 'f1',\n"
"    [0x71] = 'f2',\n"
"    [0x72] = 'f3',\n"
"    [0x73] = 'f4',\n"
"    [0x74] = 'f5',\n"
"    [0x75] = 'f6',\n"
"    [0x76] = 'f7',\n"
"    [0x77] = 'f8',\n"
"    [0x78] = 'f9',\n"
"    [0x79] = 'f10',\n"
"    [0x7A] = 'f11',\n"
"    [0x7B] = 'f12',\n"
"    [0x7C] = 'f13',\n"
"    [0x7D] = 'f14',\n"
"    [0x7E] = 'f15',\n"
"    [0x7F] = 'f16',\n"
"    [0x80] = 'f17',\n"
"    [0x81] = 'f18',\n"
"    [0x82] = 'f19',\n"
"    [0x83] = 'f20',\n"
"    [0x84] = 'f21',\n"
"    [0x85] = 'f22',\n"
"    [0x86] = 'f23',\n"
"    [0x87] = 'f24',\n"
"    [0x90] = 'numlock',\n"
"    [0x91] = 'scroll',\n"
"    [0x92] = 'oem_nec_equal',\n"
"    [0x93] = 'oem_fj_masshou',\n"
"    [0x94] = 'oem_fj_touroku',\n"
"    [0x95] = 'oem_fj_loya',\n"
"    [0x96] = 'oem_fj_roya',\n"
"    [0xA0] = 'lshift',\n"
"    [0xA1] = 'rshift',\n"
"    [0xA2] = 'lcontrol',\n"
"    [0xA3] = 'rcontrol',\n"
"    [0xA4] = 'lmenu',\n"
"    [0xA5] = 'rmenu',\n"
"    [0xA6] = 'browser_back',\n"
"    [0xA7] = 'browser_forward',\n"
"    [0xA8] = 'browser_refresh',\n"
"    [0xA9] = 'browser_stop',\n"
"    [0xAA] = 'browser_search',\n"
"    [0xAB] = 'browser_favorites',\n"
"    [0xAC] = 'browser_home',\n"
"    [0xAD] = 'volume_mute',\n"
"    [0xAE] = 'volume_down',\n"
"    [0xAF] = 'volume_up',\n"
"    [0xB0] = 'media_next_track',\n"
"    [0xB1] = 'media_prev_track',\n"
"    [0xB2] = 'media_stop',\n"
"    [0xB3] = 'media_play_pause',\n"
"    [0xB4] = 'launch_mail',\n"
"    [0xB5] = 'launch_media_select',\n"
"    [0xB6] = 'launch_app1',\n"
"    [0xB7] = 'launch_app2',\n"
"    [0xBA] = 'oem_1',\n"
"    [0xBB] = 'oem_plus',\n"
"    [0xBC] = 'oem_comma',\n"
"    [0xBD] = 'oem_minus',\n"
"    [0xBE] = 'oem_period',\n"
"    [0xBF] = 'oem_2',\n"
"    [0xC0] = 'oem_3',\n"
"    [0xDB] = 'oem_4',\n"
"    [0xDC] = 'oem_5',\n"
"    [0xDD] = 'oem_6',\n"
"    [0xDE] = 'oem_7',\n"
"    [0xDF] = 'oem_8',\n"
"    [0xE1] = 'oem_ax',\n"
"    [0xE2] = 'oem_102',\n"
"    [0xE3] = 'ico_help',\n"
"    [0xE4] = 'ico_00',\n"
"    [0xE5] = 'processkey',\n"
"    [0xE6] = 'ico_clear',\n"
"    [0xE7] = 'packet',\n"
"    [0xE9] = 'oem_reset',\n"
"    [0xEA] = 'oem_jump',\n"
"    [0xEB] = 'oem_pa1',\n"
"    [0xEC] = 'oem_pa2',\n"
"    [0xED] = 'oem_pa3',\n"
"    [0xEE] = 'oem_wsctrl',\n"
"    [0xEF] = 'oem_cusel',\n"
"    [0xF0] = 'oem_attn',\n"
"    [0xF1] = 'oem_finish',\n"
"    [0xF2] = 'oem_copy',\n"
"    [0xF3] = 'oem_auto',\n"
"    [0xF4] = 'oem_enlw',\n"
"    [0xF5] = 'oem_backtab',\n"
"    [0xF6] = 'attn',\n"
"    [0xF7] = 'crsel',\n"
"    [0xF8] = 'exsel',\n"
"    [0xF9] = 'ereof',\n"
"    [0xFA] = 'play',\n"
"    [0xFB] = 'zoom',\n"
"    [0xFC] = 'noname',\n"
"    [0xFD] = 'pa1',\n"
"    [0xFE] = 'oem_clear',\n"
"    [  48] = '\\048',\n"
"    [  49] = '\\049',\n"
"    [  50] = '\\050',\n"
"    [  51] = '\\051',\n"
"    [  52] = '\\052',\n"
"    [  53] = '\\053',\n"
"    [  54] = '\\054',\n"
"    [  55] = '\\055',\n"
"    [  56] = '\\056',\n"
"    [  57] = '\\057',\n"
"    [  65] = '\\065',\n"
"    [  66] = '\\066',\n"
"    [  67] = '\\067',\n"
"    [  68] = '\\068',\n"
"    [  69] = '\\069',\n"
"    [  70] = '\\070',\n"
"    [  71] = '\\071',\n"
"    [  72] = '\\072',\n"
"    [  73] = '\\073',\n"
"    [  74] = '\\074',\n"
"    [  75] = '\\075',\n"
"    [  76] = '\\076',\n"
"    [  77] = '\\077',\n"
"    [  78] = '\\078',\n"
"    [  79] = '\\079',\n"
"    [  80] = '\\080',\n"
"    [  81] = '\\081',\n"
"    [  82] = '\\082',\n"
"    [  83] = '\\083',\n"
"    [  84] = '\\084',\n"
"    [  85] = '\\085',\n"
"    [  86] = '\\086',\n"
"    [  87] = '\\087',\n"
"    [  88] = '\\088',\n"
"    [  89] = '\\089',\n"
"    [  90] = '\\090',\n"
"}\n");