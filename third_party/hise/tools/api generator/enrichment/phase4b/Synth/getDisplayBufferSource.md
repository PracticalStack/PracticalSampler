Synth::getDisplayBufferSource(String name) -> ScriptObject

Thread safety: INIT -- checks objectsCanBeCreated() (onInit only). Has WARN_IF_AUDIO_THREAD guard.
Returns a DisplayBufferSource handle for the named processor that holds DisplayBuffer data.
Uses owner-rooted search. Verifies the processor has DisplayBuffer data objects.

Dispatch/mechanics:
  Processor::Iterator<ProcessorWithExternalData>(owner)
  -> matches by processor ID
  -> checks getNumDataObjects(DisplayBuffer) > 0
  -> "No display buffer available" if processor found but has no display buffer data

Source:
  ScriptingApi.cpp  Synth::getDisplayBufferSource()
    -> Processor::Iterator<ProcessorWithExternalData>(owner)
    -> wraps in DisplayBufferSource handle
