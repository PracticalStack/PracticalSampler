Settings::isIppEnabled(Integer returnTrueIfMacOS) -> Integer

Thread safety: SAFE
Checks whether fast FFT acceleration is available. On Windows, returns a compile-time
constant reflecting USE_IPP=1. On macOS/other platforms, returns returnTrueIfMacOS
(pass true since macOS has vDSP, Apple's equivalent fast FFT).

Dispatch/mechanics:
  Windows: compile-time #if USE_IPP -> true/false
  Other: returns returnTrueIfMacOS parameter directly

Source:
  ScriptingApi.cpp  Settings::isIppEnabled()
    -> #if JUCE_WINDOWS: #if USE_IPP return true, else return false
    -> #else: return returnTrueIfMacOS
