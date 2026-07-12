LorisManager::get(String optionId) -> Double

Thread safety: UNSAFE -- string operations and DLL/library function call.
Returns the current value of a Loris analysis option as a number. Boolean options
return 0.0 or 1.0. The timedomain option returns an internal numeric code.
Options: "timedomain", "enablecache", "windowwidth", "freqfloor", "ampfloor",
"sidelobes", "freqdrift", "hoptime", "croptime", "bwregionwidth".

Pair with:
  set -- configure the option value

Source:
  ScriptLorisManager.cpp  ScriptLorisManager::get()
    -> LorisManager::get() -> loris_get() via C API
