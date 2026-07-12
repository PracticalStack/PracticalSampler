## set

**Examples:**

```javascript:set-velo-zones
// Title: Assigning velocity zones to sorted samples
// Context: After sorting samples by peak loudness, assign non-overlapping
// velocity ranges so quieter samples play at lower velocities.

const var sampler = Synth.getSampler("Sampler1");
const var allSamples = sampler.createSelection(".*");

const var NUM_LAYERS = 4;
const var LAYER_SIZE = parseInt(128 / NUM_LAYERS);

for (i = 0; i < allSamples.length; i++)
{
    local layerIndex = i % NUM_LAYERS;
    local loVel = parseInt(layerIndex * LAYER_SIZE);
    local hiVel = parseInt((layerIndex + 1) * LAYER_SIZE) - 1;

    // Set both LoVel and HiVel - order matters due to auto-clipping.
    // When widening a range, set HiVel first; when narrowing, set LoVel first.
    allSamples[i].set(Sample.LoVel, loVel);
    allSamples[i].set(Sample.HiVel, hiVel);
    // Repeat to ensure both values survive interdependent clamping
    allSamples[i].set(Sample.LoVel, loVel);
    allSamples[i].set(Sample.HiVel, hiVel);
}

Sampler.refreshInterface();
```

```json:testMetadata:set-velo-zones
{
  "testable": false,
  "skipReason": "Requires a loaded sampler with samples in the sample map"
}
```

```javascript:set-loop-config
// Title: Configuring loop points with dynamic range bounds
// Context: When enabling looping on an imported sample, set the loop end
// to the maximum valid value and apply a crossfade proportional to the
// available range.

const var sampler = Synth.getSampler("Sampler1");
const var sound = sampler.createSelection(".*")[0];

// Enable looping
sound.set(Sample.LoopEnabled, 1);

// Set loop end to sample end (maximum valid value)
local maxLoopEnd = sound.getRange(Sample.LoopEnd)[1];
sound.set(Sample.LoopEnd, maxLoopEnd);

// Apply crossfade as a fraction of the available range
const var XFADE_RATIO = 0.25;
local maxXFade = sound.getRange(Sample.LoopXFade)[1];
sound.set(Sample.LoopXFade, parseInt(maxXFade * XFADE_RATIO));
```

```json:testMetadata:set-loop-config
{
  "testable": false,
  "skipReason": "Requires a loaded sampler with samples in the sample map"
}
```

**Pitfalls:**
- When setting `LoVel` and `HiVel` on the same sample, the auto-clipping can fight you: setting `LoVel` above the current `HiVel` clamps it down. In production tools, set both values twice in succession to ensure both survive the interdependent range clamping.
