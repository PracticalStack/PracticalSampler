LorisManager::set(String optionId, NotUndefined newValue) -> undefined

Thread safety: UNSAFE -- string construction and DLL/library function call.
Sets a Loris analysis option. Value is converted to string internally. Options
affect subsequent analyse() calls. Invalid option ID produces a console error.
Options: "timedomain" ("seconds"/"samples"/"0to1"), "enablecache" (true/false),
"windowwidth" (0.125-4.0, default 1.0), "freqfloor" (Hz, default 40),
"ampfloor" (dB, default 90), "sidelobes" (dB, default 90), "freqdrift"
(cents, default 50), "hoptime" (s, default 0.0129), "croptime" (s, default 0.0129),
"bwregionwidth" (default 1.0).

Pair with:
  get -- read back the current value
  analyse -- options take effect on next analysis

Source:
  ScriptLorisManager.cpp  ScriptLorisManager::set()
    -> LorisManager::set() -> loris_set() via C API
    -> Options::update() applies validation and clamping (e.g. windowwidth clamped 0.125..4.0)
