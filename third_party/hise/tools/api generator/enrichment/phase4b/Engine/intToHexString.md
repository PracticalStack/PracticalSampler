Engine::intToHexString(int value) -> String

Thread safety: WARNING -- string construction
Converts integer to lowercase hex string without "0x" prefix (255 -> "ff").
Source:
  ScriptingApi.cpp  Engine::intToHexString()
    -> String::toHexString(value)
