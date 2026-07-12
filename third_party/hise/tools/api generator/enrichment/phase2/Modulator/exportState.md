## exportState

**Examples:**

```javascript:fx-lock-preset-change
// Title: Effect locking -- preserve FX settings across preset changes
// Context: An "FX Lock" feature lets users freeze their effect chain so it
// survives preset switches. Before a preset loads, each effect's state is
// captured as base64. After the preset loads, locked effects are restored.

const var eq = Synth.getEffect("ParametricEQ1");
const var comp = Synth.getEffect("Compressor1");
const var limiter = Synth.getEffect("Limiter1");

const var fxModules = [eq, comp, limiter];

// Capture state before preset change
var savedStates = {};

for (fx in fxModules)
    savedStates[fx.getId()] = fx.exportState();

// ... preset loads here ...

// Restore locked effects after preset change
for (fx in fxModules)
{
    local state = savedStates[fx.getId()];

    if (state.length > 0)
        fx.restoreState(state);
}
```
```json:testMetadata:fx-lock-preset-change
{
  "testable": false,
  "skipReason": "Requires effect modules (ParametricEQ, Compressor, Limiter) in the module tree"
}
```

```javascript:save-load-fx-to-file
// Title: Save and load FX chain to/from a JSON file on disk
// Context: Users can export their effect settings to a file and reload them
// in another session or preset. exportState produces a base64 string that
// can be stored as a JSON property.

const var eq = Synth.getEffect("ParametricEQ1");
const var comp = Synth.getEffect("Compressor1");

inline function saveFXToFile(file)
{
    local content = {};
    content[eq.getId()] = eq.exportState();
    content[comp.getId()] = comp.exportState();
    file.writeObject(content);
}

inline function loadFXFromFile(file)
{
    local content = file.loadAsObject();

    if (isDefined(content))
    {
        if (isDefined(content[eq.getId()]))
            eq.restoreState(content[eq.getId()]);

        if (isDefined(content[comp.getId()]))
            comp.restoreState(content[comp.getId()]);
    }
}
```
```json:testMetadata:save-load-fx-to-file
{
  "testable": false,
  "skipReason": "Requires effect modules in the module tree and file I/O"
}
```
