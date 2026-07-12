Synth::getSampler(String name) -> ScriptObject

Thread safety: INIT -- checks objectsCanBeCreated() (onInit only). Has WARN_IF_AUDIO_THREAD guard.
Returns a Sampler handle to the named ModulatorSampler within the parent synth's subtree.
Uses owner-rooted search. Provides methods for sample map loading, sample editing, round-robin, etc.

Source:
  ScriptingApi.cpp  Synth::getSampler()
    -> Processor::Iterator<ModulatorSampler>(owner)
    -> matches by processor ID
    -> wraps in new ScriptingObjects::ScriptingSampler
