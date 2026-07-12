## exportState

**Examples:**

```javascript:fx-lock-system
// Title: FX lock system - preserve effect states across preset changes
// Context: An FX lock feature saves the Base64 state of master/send effects
// before a preset loads, then restores them after.

const var masterFX = [Synth.getEffect("MasterEQ"),
                      Synth.getEffect("MasterComp"),
                      Synth.getEffect("MasterLimiter")];

const var sendFX = [Synth.getEffect("Delay"),
                    Synth.getEffect("ReverbA"),
                    Synth.getEffect("ReverbB")];

// Organized as two lockable groups
const var fxGroups = [masterFX, sendFX];
const var savedStates = [[], []];
const var lockEnabled = [false, false];

// Called before preset load (e.g., from a UserPresetHandler pre-load callback)
inline function saveFxStates()
{
    for (i = 0; i < fxGroups.length; i++)
    {
        if (!lockEnabled[i])
            continue;

        savedStates[i] = [];

        for (fx in fxGroups[i])
            savedStates[i].push(fx.exportState());
    }
}

// Called after preset load
inline function restoreFxStates()
{
    for (i = 0; i < fxGroups.length; i++)
    {
        if (!lockEnabled[i])
            continue;

        for (j = 0; j < fxGroups[i].length; j++)
        {
            if (savedStates[i][j].length > 0)
                fxGroups[i][j].restoreState(savedStates[i][j]);
        }
    }
}
```
```json:testMetadata:fx-lock-system
{
  "testable": false,
  "skipReason": "Defines pre/post-load callbacks for a UserPresetHandler; requires 6 named effects and external preset load events to trigger the save/restore cycle"
}
```

```javascript:save-fx-config-to-file
// Title: Save and load FX configurations to/from disk
// Context: Export all effects in a group as a JSON file keyed by effect ID,
// allowing users to save and recall custom FX setups.

const var fxList = [Synth.getEffect("MasterEQ"),
                    Synth.getEffect("MasterComp"),
                    Synth.getEffect("MasterLimiter")];

// Save: serialize each effect's state into a JSON object
inline function saveFxToFile(file)
{
    local content = {};

    for (fx in fxList)
        content[fx.getId()] = fx.exportState();

    file.writeObject(content);
}

// Load: restore each effect from the JSON object
inline function loadFxFromFile(file)
{
    local content = file.loadAsObject();

    for (fx in fxList)
    {
        local state = content[fx.getId()];

        if (isDefined(state))
            fx.restoreState(state);
    }
}
```
```json:testMetadata:save-fx-config-to-file
{
  "testable": false,
  "skipReason": "Requires File objects for disk I/O (file.writeObject, file.loadAsObject)"
}
```
