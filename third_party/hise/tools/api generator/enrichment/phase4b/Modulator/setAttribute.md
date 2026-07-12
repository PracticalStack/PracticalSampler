Modulator::setAttribute(Number index, Number value) -> undefined

Thread safety: SAFE -- uses ProcessorHelpers::getAttributeNotificationType() to
select thread-appropriate notification, safe from any callback including audio thread.
Sets a modulator attribute by parameter index. Use dynamic parameter constants
(e.g., mod.Frequency, mod.FadeIn) or getAttributeIndex() result as the index.
Bracket-write syntax mod["Frequency"] = 2.5 is equivalent.

Dispatch/mechanics:
  mod->setAttribute(index, value, getAttributeNotificationType())
    -> notification type chosen based on current thread context

Pair with:
  getAttribute -- read back the current value
  getAttributeIndex -- look up index from parameter name string

Source:
  ScriptingApiObjects.cpp:3034  setAttribute()
    -> mod->setAttribute(index, value, ProcessorHelpers::getAttributeNotificationType())
