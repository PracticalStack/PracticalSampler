## setResynthesisOptions

**Examples:**

```javascript:toggle-resample-mode
// Title: Toggle resample mode for cross-oscillator capture
// Context: Switching between normal playback and a resample configuration
//          where ForceResynthesis and a fixed RootNote enable rendering
//          one oscillator's output for import into another

const var wc = Synth.getWavetableController("WavetableSynth1");

inline function setResampleMode(controller, enabled)
{
    local opts = controller.getResynthesisOptions();

    // Base configuration for synthetic sources
    opts.PhaseMode = "StaticPhase";
    opts.CycleMultiplier = 4;
    opts.RemoveNoise = false;
    opts.UseLoris = false;

    if(enabled)
    {
        opts.ForceResynthesis = true;
        opts.RootNote = 17;  // fixed pitch for consistent cycle detection
    }
    else
    {
        opts.ForceResynthesis = false;
        opts.RootNote = -1;  // auto-detect
    }

    controller.setResynthesisOptions(opts);
};

// Enable resample mode before rendering
setResampleMode(wc, true);

// ... render and load data ...

// Restore normal mode after resampling
setResampleMode(wc, false);
```

```json:testMetadata:toggle-resample-mode
{
  "testable": false,
  "skipReason": "Requires a WavetableSynth module in the signal chain"
}
```

**Pitfalls:**
- When switching between resample and normal modes, always reset `ForceResynthesis` to `false` after the operation completes. Leaving it enabled causes unnecessary re-processing on every wavetable load, bypassing the cache.
