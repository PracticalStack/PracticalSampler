## loadAudioFilesIntoPool

**Examples:**

```javascript:load-audio-pool-init
// Title: Ensuring audio files are available at initialization
// Context: Call loadAudioFilesIntoPool() in onInit before any
// code that references audio files from scripts. In compiled
// plugins, this ensures the embedded pool references are loaded.

const var poolRefs = Engine.loadAudioFilesIntoPool();

// poolRefs is an array of all audio file references in the pool.
// The count depends on which audio files exist in the project.
Console.print("Pool refs is array: " + Array.isArray(poolRefs));
```
```json:testMetadata:load-audio-pool-init
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Pool refs is array: 1"]}
  ]
}
```
