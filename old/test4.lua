local val = require("LuaWinCon").GetClipboardText() or "No text in clipboard!"
print(val)
print(#val)
os.exit()