# Builder API Reference

Reference for using the Builder API to programmatically create HISE module trees. The Builder is a development-only tool - never use it in compiled plugins.

The full module catalog with all chains, InterfaceTypes, and resource counts is in `enrichment/resources/builder_modules.json`. Schema: `{Category: {ModuleID: {prettyName, type?, interfaces[], tables?, sliderPacks?, audioFiles?, displayBuffers?, filters?, slotFXTypes?, chains?}}}`. Each chain tuple: `[chainIndex, chainID, chainType, allowed?, mode?]`.

---

## Core Pattern

```javascript
const var builder = Synth.createBuilder();
builder.clear();        // Remove all child modules (clean slate)

// ... create modules ...

builder.flush();        // Send rebuild message to HISE UI
```

Always `clear()` first (otherwise modules accumulate on each recompile). Always `flush()` last.

---

## builder.create()

```
int create(var type, var id, int rootBuildIndex, int chainIndex)
```

| Parameter | Description |
|-----------|-------------|
| `type` | Module type constant: `builder.{Category}.{Type}` |
| `id` | Unique string identifier for the module |
| `rootBuildIndex` | Parent module's build index. `0` = master container (root). |
| `chainIndex` | Which chain slot to place the module in: `builder.ChainIndexes.{Name}` |

Returns an integer build index for referencing this module in subsequent calls.

**Nesting:** Use a parent's returned build index as `rootBuildIndex` for its children:

```javascript
var sampler = builder.create(builder.SoundGenerators.StreamingSampler,
                             "Sampler1", 0, builder.ChainIndexes.Direct);
builder.create(builder.Modulators.AHDSR, "GainEnv1",
               sampler, builder.ChainIndexes.Gain);
builder.create(builder.Effects.SimpleGain, "Mixer1",
               sampler, builder.ChainIndexes.FX);
```

---

## ChainIndexes

| Constant | Value | Purpose |
|----------|-------|---------|
| `builder.ChainIndexes.Midi` | 0 | MIDI processors |
| `builder.ChainIndexes.Gain` | 1 | Gain modulation chain |
| `builder.ChainIndexes.Pitch` | 2 | Pitch modulation chain |
| `builder.ChainIndexes.FX` | 3 | Effects chain |
| `builder.ChainIndexes.Direct` | -1 | Child sound generators |

Some modules have additional chains beyond index 3 (e.g., StreamingSampler has sample-start and group-start modulation at indexes 4 and 5). Check `builder_modules.json` for the full chain list per module.

---

## Module Catalog

### SoundGenerators

Added as children of the master container (or another synth container) using `builder.ChainIndexes.Direct`.

| BuilderPath | Common Use | Standard Chains |
|-------------|-----------|-----------------|
| `builder.SoundGenerators.StreamingSampler` | Sample playback | Midi, Gain, Pitch, FX + sample-start chains |
| `builder.SoundGenerators.SineSynth` | Simple sine tone | Midi, Gain, Pitch, FX |
| `builder.SoundGenerators.WaveSynth` | Waveform generator | Midi, Gain, Pitch, FX |
| `builder.SoundGenerators.Noise` | Noise generator | Midi, Gain, Pitch, FX |
| `builder.SoundGenerators.GlobalModulatorContainer` | Shared modulator host | Midi + "Global Modulators" (index 1) |
| `builder.SoundGenerators.SynthGroup` | Container for layered synths | Midi, Gain, Pitch, FX |
| `builder.SoundGenerators.SilentSynth` | Silent placeholder (for effects-only chains) | Midi, Gain, Pitch, FX |

**GlobalModulatorContainer special case:** Child modulators go into chain index `1` (the "Global Modulators" slot - same numeric value as `ChainIndexes.Gain` but semantically different). LFOs inside a GlobalModulatorContainer run monophonically without requiring active voices.

```javascript
var gmcIndex = builder.create(builder.SoundGenerators.GlobalModulatorContainer,
                              "GMC1", 0, builder.ChainIndexes.Direct);
builder.create(builder.Modulators.LFO, "TestLFO", gmcIndex, 1);
```

### MidiProcessors

Added to a parent module's MIDI chain using `builder.ChainIndexes.Midi`.

| BuilderPath | Common Use |
|-------------|-----------|
| `builder.MidiProcessors.ScriptProcessor` | Custom MIDI processing via script |
| `builder.MidiProcessors.MidiPlayer` | MIDI sequence playback and recording |
| `builder.MidiProcessors.Transposer` | Note transposition |
| `builder.MidiProcessors.Arpeggiator` | Arpeggio pattern generator |
| `builder.MidiProcessors.ReleaseTrigger` | Triggers notes on key release |

### Modulators

Added to Gain, Pitch, or other modulation chains of a parent module.

**Voice-start modulators** (evaluate once per note-on):

| BuilderPath | Common Use |
|-------------|-----------|
| `builder.Modulators.Velocity` | Velocity-to-gain mapping |
| `builder.Modulators.KeyNumber` | Key-to-parameter mapping |
| `builder.Modulators.Random` | Random per-note modulation |
| `builder.Modulators.Constant` | Fixed modulation value |
| `builder.Modulators.ArrayModulator` | SliderPack-driven per-note values |

**Time-variant modulators** (continuous modulation):

| BuilderPath | Common Use |
|-------------|-----------|
| `builder.Modulators.LFO` | Low-frequency oscillator |
| `builder.Modulators.MidiController` | CC-driven modulation |
| `builder.Modulators.PitchWheel` | Pitch wheel modulation |
| `builder.Modulators.MacroModulator` | Macro control-driven modulation |

**Envelope modulators** (per-voice amplitude/pitch shaping):

| BuilderPath | Common Use |
|-------------|-----------|
| `builder.Modulators.AHDSR` | Classic AHDSR envelope |
| `builder.Modulators.FlexAHDSR` | AHDSR with per-segment curves |
| `builder.Modulators.SimpleEnvelope` | Basic attack/release envelope |
| `builder.Modulators.TableEnvelope` | Table-driven envelope shape |

### Effects

Added to a parent module's FX chain using `builder.ChainIndexes.FX`.

| BuilderPath | Common Use |
|-------------|-----------|
| `builder.Effects.SimpleGain` | Volume/pan control (often used as mixer) |
| `builder.Effects.PolyphonicFilter` | Per-voice filter with modulation chains |
| `builder.Effects.Convolution` | Impulse response reverb |
| `builder.Effects.SimpleReverb` | Algorithmic reverb |
| `builder.Effects.Delay` | Delay effect |
| `builder.Effects.Dynamics` | Compressor/gate |
| `builder.Effects.Analyser` | FFT/oscilloscope display |
| `builder.Effects.SlotFX` | Swappable effect slot (hardcoded networks) |

---

## builder.get()

Converts a build index into a typed script reference for post-creation operations.

```
var ref = builder.get(buildIndex, builder.InterfaceTypes.{Type})
```

| InterfaceType | Purpose | Example use |
|---------------|---------|------------|
| `ChildSynth` | Sound generator reference | `.setBypassed()`, `.getAttribute()` |
| `Sampler` | Sampler-specific operations | `.loadSampleMap("name")` |
| `MidiPlayer` | MidiPlayer operations | `.create()`, `.setSyncToMasterClock()` |
| `MidiProcessor` | Generic MIDI processor | `.setAttribute()` |
| `Effect` | Effect reference | `.getAttribute()`, `.setBypassed()` |
| `Modulator` | Modulator reference | `.setAttribute()`, `.setIntensity()` |
| `TableProcessor` | Table editing | `.setTablePoint()` |
| `SliderPackProcessor` | SliderPack editing | `.setSliderPackData()` |
| `RoutingMatrix` | Routing configuration | `.setChannel()` |
| `AudioSampleProcessor` | Audio file loading | `.setFile()` |
| `SlotFX` | Effect slot swapping | `.setEffect()` |

```javascript
var sampler = builder.create(builder.SoundGenerators.StreamingSampler,
                             "Sampler1", 0, builder.ChainIndexes.Direct);
var asSampler = builder.get(sampler, builder.InterfaceTypes.Sampler);
asSampler.loadSampleMap("MySampleMap");
```

---

## builder.setAttributes()

Sets module parameters using a JSON object. Only specify non-default values.

```javascript
var ahdsr = builder.create(builder.Modulators.AHDSR, "GainEnv1",
                           sampler, builder.ChainIndexes.Gain);
builder.setAttributes(ahdsr, {
    "Attack": 8000,
    "Release": 100.0
});
```

---

## builder.clearChildren()

Removes all child processors from a specific chain of a module. Useful for removing default modules (e.g., the SimpleEnvelope auto-inserted in new synths).

```javascript
builder.clearChildren(sampler, builder.ChainIndexes.Gain);
```

---

## builder.connectToScript()

Loads an external script file into a ScriptProcessor created by the Builder.

```javascript
var sp = builder.create(builder.MidiProcessors.ScriptProcessor,
                        "MyScript", sampler, builder.ChainIndexes.Midi);
builder.connectToScript(sp, "{PROJECT_FOLDER}Scripts/MyProcessor.js");
```

The file reference syntax matches HISEScript `include` statements.
