Synth::getEffect(String name) -> ScriptObject

Thread safety: INIT -- checks objectsCanBeCreated() (onInit only). Has WARN_IF_AUDIO_THREAD guard.
Returns a ScriptEffect handle to the named effect processor within the parent synth's subtree.
Uses owner-rooted search. Store the reference in a const var at the top level of your script.

Pair with:
  getAllEffects -- get multiple effects by wildcard pattern
  addEffect / removeEffect -- dynamic effect chain modification

Source:
  ScriptingApi.cpp  Synth::getEffect()
    -> Processor::Iterator<EffectProcessor>(owner)
    -> matches by processor ID
    -> wraps in new ScriptingObjects::ScriptingEffect
