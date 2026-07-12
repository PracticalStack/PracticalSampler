Engine::getOS() -> String

Thread safety: WARNING -- string construction from compile-time literal
Returns "WIN", "OSX", or "LINUX" based on compile-time preprocessor guards.
Source:
  ScriptingApi.cpp  Engine::getOS()
    -> #if JUCE_WINDOWS: "WIN" / #if JUCE_MAC: "OSX" / else: "LINUX"
