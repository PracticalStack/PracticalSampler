## loadIntoBufferArray

**Examples:**

```javascript:load-buffer-pitch
// Title: Pitch detection from sample audio data
// Context: Load sample audio into a buffer, detect its fundamental
// frequency, and set the root note accordingly. Useful for user-import
// workflows where sample metadata is missing.

const var sampler = Synth.getSampler("Sampler1");
const var sound = sampler.createSelection(".*")[0];

// loadIntoBufferArray returns a flat array: [mic1_L, mic1_R, mic2_L, ...]
// For a mono single-mic sample, index 0 is the only channel.
const var buf = sound.loadIntoBufferArray()[0];

// Detect pitch using the sample's own sample rate
const var freq = buf.detectPitch(sound.getSampleRate());

// Convert frequency to nearest MIDI note
for (i = 0; i < 128; i++)
{
    if (Engine.getFrequencyForMidiNoteNumber(i) > freq)
    {
        local detectedNote = i - 1;
        local currentRoot = sound.get(Sample.Root);

        if (currentRoot != detectedNote)
        {
            Console.print("Pitch: " + Engine.getMidiNoteName(currentRoot)
                + " -> " + Engine.getMidiNoteName(detectedNote));
            sound.set(Sample.Root, detectedNote);
        }
        break;
    }
}
```

```json:testMetadata:load-buffer-pitch
{
  "testable": false,
  "skipReason": "Requires a loaded sampler with samples in the sample map"
}
```

```javascript:load-buffer-peak
// Title: Measuring peak loudness for velocity zone sorting
// Context: Analyze peak amplitude of each sample to sort them by
// loudness before assigning velocity layers.

const var sampler = Synth.getSampler("Sampler1");
const var selection = sampler.createSelection(".*");

for (s in selection)
{
    // getMagnitude returns the absolute peak value of the buffer
    local buf = s.loadIntoBufferArray()[0];
    local peakDb = Engine.getDecibelsForGainFactor(buf.getMagnitude());
    Console.print(s.get(Sample.FileName) + ": " + peakDb + " dB");
}
```

```json:testMetadata:load-buffer-peak
{
  "testable": false,
  "skipReason": "Requires a loaded sampler with samples in the sample map"
}
```

**Pitfalls:**
- Each call loads the entire sample into memory as Buffer objects. When analyzing many samples in a loop, call `Engine.extendTimeOut()` periodically to prevent the script execution timeout from firing on large sample sets.
