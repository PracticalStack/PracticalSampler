Sample::loadIntoBufferArray() -> Array

Thread safety: UNSAFE -- allocates VariantBuffer objects and AudioFormatReaders for each mic position. Loads entire sample data into memory.
Loads complete audio data into an array of Buffer objects. For multi-mic samples,
returns a flat array: [mic1_L, mic1_R, mic2_L, mic2_R] for 2 stereo mic positions.
Mono mic positions contribute one buffer each.
Dispatch/mechanics:
  iterates sound->getNumMultiMicSamples()
    -> creates AudioFormatReader per mic position
    -> mono: 1 VariantBuffer; stereo: 2 VariantBuffers
    -> returns flat array of all channel buffers
Pair with:
  replaceAudioFile -- write modified buffers back to disk
Anti-patterns:
  - [BUG] No objectExists() check -- dereferences null if underlying sound was deleted
  - Loads entire sample into memory -- significant allocation for large samples
    or many mic positions
Source:
  ScriptingApiObjects.cpp  loadIntoBufferArray()
    -> iterates getNumMultiMicSamples()
    -> AudioFormatReader per mic -> VariantBuffer per channel
