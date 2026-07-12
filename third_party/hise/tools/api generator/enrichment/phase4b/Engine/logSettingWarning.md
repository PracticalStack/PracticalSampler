Engine::logSettingWarning(String methodName) -> undefined

Thread safety: UNSAFE -- String construction, console write
Internal deprecation helper. Emits "Engine.{methodName}() is deprecated. Use
Settings.{methodName}() instead." to console. No practical use for end users.
Source:
  ScriptingApi.cpp  Engine::logSettingWarning()
    -> debugToConsole(deprecation message)
