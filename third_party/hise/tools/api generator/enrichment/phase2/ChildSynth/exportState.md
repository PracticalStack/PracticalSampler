## exportState

**Examples:**

```javascript:fx-lock-snapshot
// Title: FX lock - preserving effect state across preset changes
// Context: An "FX lock" feature saves the current state of effects before a
// preset loads, then restores them afterward. exportState captures the
// complete processor state as a base64 string that can be stored in a
// variable or serialized to JSON.

const var fx = Synth.getEffect("MasterEQ");
const var compressor = Synth.getEffect("MasterComp");

// Store state objects for each lockable effect
var fxStates = [{ FX: fx, State: "" },
                { FX: compressor, State: "" }];

// Before a preset loads: snapshot states of locked effects
inline function onPresetPreLoad()
{
    for (item in fxStates)
        item.State = item.FX.exportState();
}

// After a preset loads: restore the snapshots
inline function onPresetPostLoad()
{
    for (item in fxStates)
    {
        if (item.State.length > 0)
            item.FX.restoreState(item.State);
    }
}
```
```json:testMetadata:fx-lock-snapshot
{
  "testable": false,
  "skipReason": "Defines preset lifecycle callbacks that require manual invocation; requires named effects in the module tree"
}
```

```javascript:save-fx-chain-json
// Title: Saving effect chain state to a JSON file
// Context: Export each effect's base64 state into a JSON object for
// saving/loading custom FX chain configurations to disk.

const var effects = [Synth.getEffect("EQ"),
                     Synth.getEffect("Compressor"),
                     Synth.getEffect("Reverb")];

inline function saveFXChain(file)
{
    local content = {};

    for (fx in effects)
        content[fx.getId()] = fx.exportState();

    file.writeObject(content);
}
```
```json:testMetadata:save-fx-chain-json
{
  "testable": false,
  "skipReason": "Requires named effects in the module tree and a File object for disk I/O"
}
```

**Cross References:**
- `ChildSynth.restoreState`
- `Effect.exportState`
