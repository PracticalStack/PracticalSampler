## restoreState

**Examples:**

```javascript:fx-lock-restore
// Title: Restoring effect state after preset load (FX lock)
// Context: After a preset loads, restore previously saved base64 state
// strings to preserve the user's FX settings. Also update any connected
// UI components to reflect the restored processor state.

const var eq = Synth.getEffect("ChannelEQ");

// Saved state from a previous exportState() call
var savedEqState = "";

inline function lockEQ()
{
    savedEqState = eq.exportState();
}

inline function restoreLockedEQ()
{
    if (savedEqState.length > 0)
    {
        eq.restoreState(savedEqState);

        // After restoring, sync the UI with the processor state
        uph.updateConnectedComponentsFromModuleState();
    }
}
```
```json:testMetadata:fx-lock-restore
{
  "testable": false,
  "skipReason": "Defines preset lifecycle callbacks (lockEQ/restoreLockedEQ) that require manual invocation around preset load; references undefined uph (UserPresetHandler)"
}
```

```javascript:builder-default-state
// Title: Restoring default state during Builder construction
// Context: When programmatically building the module tree with the Builder API,
// newly created effects can be initialized to a known default state
// using a pre-captured base64 string.

const var b = Synth.createBuilder();

// A base64 string captured from a configured EQ module
const var DEFAULT_EQ_STATE = "516.3oc..."; // (truncated for brevity)

// During Builder construction:
b.flush("const var eq = b.create(b.Effects.CurveEq, \"MasterEQ\", 0, b.ChainIndexes.FX);", []);
b.flush("b.get(eq, b.InterfaceTypes.Effect).restoreState(DEFAULT_EQ_STATE);", []);
```
```json:testMetadata:builder-default-state
{
  "testable": false,
  "skipReason": "Pseudo-code showing Builder API pattern with truncated base64 string; not a complete runnable script"
}
```

**Cross References:**
- `ChildSynth.exportState`
- `Effect.restoreState`
