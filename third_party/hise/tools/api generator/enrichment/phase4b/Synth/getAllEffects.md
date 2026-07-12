Synth::getAllEffects(String regex) -> Array

Thread safety: INIT -- checks objectsCanBeCreated() (onInit only). Allocates ScriptEffect wrappers on heap.
Returns an array of ScriptEffect handles for all effects in the parent synth's subtree matching
the wildcard pattern. Uses owner-rooted search. Pattern supports * glob (not full regex).

Anti-patterns:
  - Do NOT call outside onInit -- silently returns an empty object (not an array) instead
    of reporting an error. Other get*() methods report a clear "onInit only" message.

Source:
  ScriptingApi.cpp  Synth::getAllEffects()
    -> Processor::Iterator<EffectProcessor>(owner)
    -> RegexFunctions::matchesWildcard(processorId, regex)
    -> wraps each match in new ScriptingObjects::ScriptingEffect
