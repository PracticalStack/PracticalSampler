AudioSampleProcessor::getAudioFile(Number slotIndex) -> ScriptObject

Thread safety: UNSAFE -- allocates a new ScriptAudioFile object on the heap.
Returns an AudioFile scripting object for the audio file data at the given slot index.
AudioSampleProcessor modules have exactly one slot (index 0). The returned AudioFile
provides access to sample data, content change callbacks, and buffer manipulation.
Required setup:
  const var asp = Synth.getAudioSampleProcessor("AudioLooper1");
  const var af = asp.getAudioFile(0);
Dispatch/mechanics:
  dynamic_cast<ProcessorWithExternalData*>(processor) -> new ScriptAudioFile(scriptProcessor, slotIndex, ed)
Pair with:
  setFile -- load a file before accessing its AudioFile data
Anti-patterns:
  - Do NOT pass slotIndex other than 0 -- AudioSampleProcessor always has exactly one
    slot. Other indices create a handle to a non-existent slot with undefined behavior.
Source:
  ScriptingApiObjects.cpp:4997  getAudioFile() -> new ScriptAudioFile(getScriptProcessor(), slotIndex, ed)
