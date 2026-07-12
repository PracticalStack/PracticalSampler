Synth::getSlotFX(String name) -> ScriptObject

Thread safety: INIT -- checks objectsCanBeCreated() (onInit only). Has WARN_IF_AUDIO_THREAD guard.
Returns a ScriptSlotFX handle using a dual search: first HotswappableProcessor (traditional SlotFX),
then falls back to DspNetwork::Holder (scriptnode-based slot). Uses owner-rooted search.

Source:
  ScriptingApi.cpp  Synth::getSlotFX()
    -> Processor::Iterator<HotswappableProcessor>(owner) -- first search
    -> if not found: Processor::Iterator<DspNetwork::Holder>(owner) -- fallback
    -> wraps in new ScriptingObjects::ScriptingSlotFX
