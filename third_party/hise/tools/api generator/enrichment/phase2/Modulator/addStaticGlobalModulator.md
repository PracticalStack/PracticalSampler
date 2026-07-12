## addStaticGlobalModulator

**Examples:**

```javascript:static-velocity-connection
// Title: Create a static connection for a velocity source
// Context: Voice-start sources (velocity, key number, random) produce a single
// value at note-on that does not change during the note. Using the static
// variant avoids the CPU cost of per-block polling for a value that is constant.

const var globalVelocity = Synth.getModulator("GlobalVelocity1");
const var filter = Synth.getEffect("PolyFilter1");
const var GAIN_CHAIN = 0;

// Static receiver: samples the source value once at voice start
const var velMod = filter.addStaticGlobalModulator(
    GAIN_CHAIN, globalVelocity, "Velocity_to_Filter"
);

velMod.setIntensity(0.7);

// To remove the connection later:
// Synth.removeModulator(velMod);
```
```json:testMetadata:static-velocity-connection
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer with a velocity modulator and a PolyFilter effect"
}
```

**Pitfalls:**
- Static receivers only sample the source at voice start. Using `addStaticGlobalModulator` with a continuously changing source (LFO, envelope) will capture a single snapshot value per note, which is usually not the desired behavior.
