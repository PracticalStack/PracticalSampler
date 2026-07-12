## exportState

**Examples:**

```javascript:effect-lock-across-presets
// Title: Effect locking -- preserve processor state across preset changes
// Context: A preset system that allows users to "lock" certain effects so
// they persist when loading a new preset. The state is captured as base64
// before the preset loads and restored after.

const var NUM_FX_SLOTS = 3;
const var fxModules = [];
const var fxLockStates = [];
const var fxSavedStates = [];

for (i = 0; i < NUM_FX_SLOTS; i++)
{
    fxModules.push(Synth.getEffect("FX" + (i + 1)));
    fxLockStates.push(false);
    fxSavedStates.push("");
}

// Call before loading a new preset
inline function captureLockedStates()
{
    for (i = 0; i < NUM_FX_SLOTS; i++)
    {
        // Only capture state for locked slots
        fxSavedStates[i] = fxLockStates[i] ? fxModules[i].exportState() : "";
    }
}

// Call after loading a new preset
inline function restoreLockedStates()
{
    for (i = 0; i < NUM_FX_SLOTS; i++)
    {
        if (fxSavedStates[i].length > 0)
        {
            Console.print("Restoring locked state for " + fxModules[i].getId());
            fxModules[i].restoreState(fxSavedStates[i]);
        }
    }
}
```
```json:testMetadata:effect-lock-across-presets
{
  "testable": false,
  "skipReason": "Requires existing effect modules in the module tree; utility functions defined but not invoked"
}
```

```javascript:save-chain-to-file
// Title: Saving effect chain state to a JSON file
// Context: Export the state of multiple processors for import/export
// functionality outside the standard preset system.

const var fxChain = [Synth.getEffect("EQ1"),
                     Synth.getEffect("Compressor1"),
                     Synth.getEffect("Delay1")];

inline function saveChainToFile(file)
{
    local content = {};

    for (fx in fxChain)
        content[fx.getId()] = fx.exportState();

    file.writeObject(content);
}
```
```json:testMetadata:save-chain-to-file
{
  "testable": false,
  "skipReason": "Requires existing effect modules and file system access"
}
```
