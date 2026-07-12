Synth::setAttribute(Integer attributeIndex, Double newAttribute) -> undefined

Thread safety: UNSAFE -- calls Processor::setAttribute() with sendNotification, which dispatches change messages involving string lookups and var comparisons.
Sets a parameter on the parent synth by attribute index. Standard ModulatorSynth indices:
0=Gain (0.0-1.0), 1=Balance (-100 to 100), 2=VoiceLimit, 3=KillFadeTime.

Dispatch/mechanics:
  owner->setAttribute(attributeIndex, newAttribute, sendNotification)
  -> setInternalAttribute() writes value
  -> dispatches change notification to UI and connected listeners

Pair with:
  getAttribute -- read the same parameter by index

Anti-patterns:
  - No validation on attributeIndex -- out-of-range indices may silently be ignored or write
    to invalid parameter slots depending on the synth subclass.

Source:
  ScriptingApi.cpp  Synth::setAttribute()
    -> owner->setAttribute(attributeIndex, newAttribute, sendNotification)
