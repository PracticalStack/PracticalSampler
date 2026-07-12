## restoreState

**Examples:**

```javascript:restore-builder-module-state
// Title: Restore module state during Builder API initialization
// Context: When using the Builder API to create modules, restoreState can
// apply a pre-configured state captured from the HISE IDE. This avoids
// setting dozens of individual attributes manually.

const var builder = Synth.createBuilder();

// Create an EQ module via the Builder
const var eqModule = builder.create("ParametricEQ", "ChannelEQ", parentChain, -1);
const var eq = builder.get(eqModule, builder.InterfaceTypes.Effect);

// Apply a pre-captured base64 state (exported from HISE IDE)
const var EQ_DEFAULT_STATE = "782.3oc0V0saaCDE..."; // truncated for brevity
eq.restoreState(EQ_DEFAULT_STATE);

// The EQ now has all bands, frequencies, and gains pre-configured
eq.setBypassed(true); // start bypassed until the user enables it
```
```json:testMetadata:restore-builder-module-state
{
  "testable": false,
  "skipReason": "Requires Builder API context with a parent chain; uses truncated base64 state"
}
```

**Cross References:**
- `Modulator.exportState` -- produces the base64 string consumed by this method
