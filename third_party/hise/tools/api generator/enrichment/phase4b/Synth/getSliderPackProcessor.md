Synth::getSliderPackProcessor(String name) -> ScriptObject

Thread safety: INIT -- checks objectsCanBeCreated() (onInit only). Has WARN_IF_AUDIO_THREAD guard.
Returns a ScriptSliderPackProcessor handle for the named ExternalDataHolder. Uses owner-rooted
search. Does NOT verify the processor actually has slider pack data -- matches any ExternalDataHolder.

Anti-patterns:
  - Do NOT assume the returned handle points to a slider pack processor -- it matches any
    ExternalDataHolder by name. If the processor only has Table data, methods may fail.

Source:
  ScriptingApi.cpp  Synth::getSliderPackProcessor()
    -> Processor::Iterator<ExternalDataHolder>(owner)
    -> matches by processor ID (no data type check)
    -> wraps in new ScriptingObjects::ScriptSliderPackProcessor
