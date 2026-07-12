Synth::getAllModulators(String regex) -> Array

Thread safety: UNSAFE -- allocates ScriptingModulator wrappers on the heap. No objectsCanBeCreated() guard.
Returns an array of ScriptModulator handles for all modulators in the ENTIRE module tree
(global-rooted search from getMainSynthChain) matching the wildcard pattern.

Dispatch/mechanics:
  Processor::Iterator<Modulator>(getMainSynthChain()) -- searches entire tree
  RegexFunctions::matchesWildcard(processorId, regex)
  Wraps each match in new ScriptingObjects::ScriptingModulator

Anti-patterns:
  - Do NOT confuse with getModulator (owner-rooted). getAllModulators returns modulators
    from ALL synths in the project, not just the parent synth's subtree.
  - Do NOT call at runtime -- allocates wrapper objects on every call. Cache results in onInit.

Source:
  ScriptingApi.cpp  Synth::getAllModulators()
    -> Processor::Iterator<Modulator>(getMainSynthChain())
    -> wraps matches in new ScriptingObjects::ScriptingModulator
