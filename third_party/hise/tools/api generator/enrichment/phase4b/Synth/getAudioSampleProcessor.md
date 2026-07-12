Synth::getAudioSampleProcessor(String name) -> ScriptObject

Thread safety: UNSAFE -- allocates a ScriptAudioSampleProcessor wrapper on the heap. Has WARN_IF_AUDIO_THREAD guard.
Returns a ScriptAudioSampleProcessor handle for the named processor that holds AudioFile data.
Uses owner-rooted search (parent synth subtree only). Requires AudioFile data on the processor.

Dispatch/mechanics:
  Processor::Iterator<ProcessorWithExternalData>(owner)
  -> matches by processor ID, then checks getNumDataObjects(AudioFile) > 0
  -> if processor exists but has no AudioFile data, silently skipped

Anti-patterns:
  - Do NOT call outside onInit -- unlike most get*() methods, this one has no explicit
    onInit restriction but allocates on the heap. Always cache in onInit.
  - If the processor exists but has no AudioFile data, the error says "not found" without
    distinguishing "wrong type" from "not found".

Source:
  ScriptingApi.cpp  Synth::getAudioSampleProcessor()
    -> Processor::Iterator<ProcessorWithExternalData>(owner)
    -> checks getNumDataObjects(ExternalData::DataType::AudioFile) > 0
    -> wraps in new ScriptingObjects::ScriptAudioSampleProcessor
