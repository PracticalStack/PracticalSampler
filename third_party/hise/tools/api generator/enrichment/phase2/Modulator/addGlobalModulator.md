## addGlobalModulator

**Examples:**

```javascript:dynamic-global-lfo-connection
// Title: Create a dynamic modulation connection from a global LFO to a filter
// Context: A modulation matrix allows users to route global sources to targets
// at runtime. This creates a time-variant receiver that continuously tracks
// the source modulator's output.

const var globalLFO = Synth.getModulator("GlobalLFO1");
const var filter = Synth.getEffect("PolyFilter1");
const var GAIN_CHAIN = 0;

// Validate the target before creating the connection
if (filter.getId() != "")
{
    // Create a receiver that continuously tracks the LFO output
    const var receiver = filter.addGlobalModulator(GAIN_CHAIN, globalLFO, "LFO_to_Filter");
    receiver.setIntensity(0.5);
}
```
```json:testMetadata:dynamic-global-lfo-connection
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer with LFO and a PolyFilter effect"
}
```

```javascript:static-vs-dynamic-routing
// Title: Choose static vs dynamic routing based on source type
// Context: LFOs and envelopes need continuous tracking (addGlobalModulator),
// while velocity and random sources only produce a value at voice start
// (addStaticGlobalModulator). Some targets may also force static routing.

const var sourceMod = Synth.getModulator("GlobalLFO1");
const var target = Synth.getEffect("PolyFilter1");
const var GAIN_CHAIN = 0;

// Use static routing for voice-start sources or targets that don't
// support continuous modulation; dynamic routing otherwise
const var useStatic = false;
var newMod;

if (useStatic)
    newMod = target.addStaticGlobalModulator(GAIN_CHAIN, sourceMod, "Mod_Connection");
else
    newMod = target.addGlobalModulator(GAIN_CHAIN, sourceMod, "Mod_Connection");

newMod.setIntensity(1.0);

// To remove the connection later:
// Synth.removeModulator(newMod);
```
```json:testMetadata:static-vs-dynamic-routing
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer with LFO and a PolyFilter effect"
}
```

**Pitfalls:**
- The `globalMod` parameter must reference a modulator inside a `GlobalModulatorContainer`. Passing a regular modulator will fail silently or produce undefined behavior.
- Always validate the target module before calling: check `target.getId() != ""` to ensure the module reference is valid. Invalid targets cause runtime errors.
