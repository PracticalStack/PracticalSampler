Settings::setEnableDebugMode(Integer shouldBeEnabled) -> undefined

Thread safety: UNSAFE -- starts or stops the debug logger, involving file I/O
Enables or disables the debug logger. When enabled, HISE logs detailed diagnostic
information to a file for troubleshooting.

Pair with:
  crashAndBurn -- enable debug logging before crash testing

Source:
  ScriptingApi.cpp  Settings::setEnableDebugMode()
    -> mc->getDebugLogger().startLogging() / stopLogging()
