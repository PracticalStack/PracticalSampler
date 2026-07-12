## loadData

**Examples:**

```javascript:load-generated-waveform
// Title: Load a generated single-cycle waveform as wavetable
// Context: Creating basic waveforms (sine, saw, square) from scratch
//          and loading them as single-cycle wavetables

const var wc = Synth.getWavetableController("WavetableSynth1");

// Configure options for clean synthetic sources
var opts = wc.getResynthesisOptions();
opts.RemoveNoise = false;
opts.UseLoris = false;
wc.setResynthesisOptions(opts);

// Generate a single saw cycle in a Buffer
const var NUM_SAMPLES = 2048;
const var cycle = Buffer.create(NUM_SAMPLES);

for(i = 0; i < NUM_SAMPLES; i++)
    cycle[i] = 2.0 * Math.fmod((i + NUM_SAMPLES / 2) / NUM_SAMPLES, 1.0) - 1.0;

// Load the buffer with sample rate and loop range
wc.loadData(cycle, 48000.0, [0, NUM_SAMPLES]);
```

```json:testMetadata:load-generated-waveform
{
  "testable": false,
  "skipReason": "Requires a WavetableSynth module in the signal chain"
}
```

```javascript:clear-wavetable-data
// Title: Clear wavetable data
// Context: Resetting a wavetable oscillator to empty state

const var wc = Synth.getWavetableController("WavetableSynth1");

// Pass an empty array to clear the wavetable
wc.loadData([], 0, []);
```

```json:testMetadata:clear-wavetable-data
{
  "testable": false,
  "skipReason": "Requires a WavetableSynth module in the signal chain"
}
```

```javascript:load-rendered-audio-channels
// Title: Load rendered audio channels into a wavetable
// Context: Capturing rendered audio output and importing it as
//          wavetable data for cross-oscillator resampling

const var sourceWc = Synth.getWavetableController("WavetableSynth1");
const var targetWc = Synth.getWavetableController("WavetableSynth2");

// Build a MIDI event list for rendering
var list = [];
var noteOn = Engine.createMessageHolder();
var noteOff = Engine.createMessageHolder();

noteOn.setType(Message.NoteOn);
noteOff.setType(Message.NoteOff);
noteOn.setChannel(1);
noteOff.setChannel(1);
noteOn.setVelocity(127);
noteOn.setNoteNumber(60);
noteOff.setNoteNumber(60);
noteOff.setTimestamp(2048 * 64);

list.push(noteOn);
list.push(noteOff);

// Render audio and load the result into the target oscillator
Engine.renderAudio(list, function[targetWc](data, funky)
{
    // data.channels is an array of Buffers (one per channel)
    targetWc.loadData(data.channels, Engine.getSampleRate(), []);
});
```

```json:testMetadata:load-rendered-audio-channels
{
  "testable": false,
  "skipReason": "Requires two WavetableSynth modules and Engine.renderAudio"
}
```
