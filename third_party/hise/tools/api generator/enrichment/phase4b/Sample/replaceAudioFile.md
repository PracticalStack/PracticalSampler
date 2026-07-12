Sample::replaceAudioFile(var audioData) -> bool

Thread safety: UNSAFE -- writes to audio files on disk (I/O). No explicit thread safety guards -- caller must ensure no voices are playing.
Replaces audio data of this sample's file(s) on disk. Array must contain one
Buffer per channel across all mic positions, matching loadIntoBufferArray()
structure. All buffers must have same length. Returns true on success.
Required setup:
  var buffers = sample.loadIntoBufferArray();
  // ... modify buffers ...
  sample.replaceAudioFile(buffers);
Dispatch/mechanics:
  validates: sound exists -> audioData is array -> no monolithic storage
    -> counts channels across mic positions -> validates buffer count and lengths
    -> writes via StreamingSamplerSound::replaceAudioFile() per mic position
Pair with:
  loadIntoBufferArray -- load audio first, modify, then replace
Anti-patterns:
  - Do NOT call on monolithic samples (.ch1/.ch2 files) -- throws "Can't write
    to monolith files"
  - Does NOT kill voices or acquire locks unlike duplicateSample/deleteSample --
    caller must ensure safe state externally
  - [BUG] After "channel length mismatch" or "Invalid channel data" errors,
    execution may continue without returning in non-throwing builds
Source:
  ScriptingApiObjects.cpp  replaceAudioFile()
    -> validates array, checks no monolith, counts channels
    -> StreamingSamplerSound::replaceAudioFile() per mic position
