Synth::getTableProcessor(String name) -> ScriptObject

Thread safety: INIT -- checks objectsCanBeCreated() (onInit only). Has WARN_IF_AUDIO_THREAD guard.
Returns a ScriptTableProcessor handle for the named ExternalDataHolder. Uses owner-rooted search.
Does NOT verify the processor actually has table data -- matches any ExternalDataHolder by name.

Anti-patterns:
  - Do NOT assume the returned handle points to a table processor -- it matches any
    ExternalDataHolder by name. If the processor only has AudioFile data, table methods may fail.

Source:
  ScriptingApi.cpp  Synth::getTableProcessor()
    -> Processor::Iterator<ExternalDataHolder>(owner)
    -> matches by processor ID (no data type check)
    -> wraps in new ScriptingObjects::ScriptTableProcessor
