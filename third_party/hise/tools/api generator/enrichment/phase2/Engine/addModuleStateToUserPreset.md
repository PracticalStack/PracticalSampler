## addModuleStateToUserPreset

**Examples:**

```javascript:selective-eq-state-persistence
// Title: Selectively persisting EQ module state in user presets
// Context: When adding a module's state to user presets, pass a JSON
// object with RemovedProperties and RemovedChildElements to exclude
// noise like routing matrix data, bypass state, and network references.
// This keeps presets lean and avoids restoring unwanted state.

// Create a filter config that removes non-essential properties
var eqConfig = {
    "RemovedProperties": [
        "Type", "Network", "Bypassed", "NumFilters", "FFTEnabled"
    ],
    "RemovedChildElements": [
        "ChildProcessors", "RoutingMatrix"
    ]
};

// Register each EQ band with the same filter config
eqConfig.ID = "MasterEQ";
Engine.addModuleStateToUserPreset(eqConfig);

eqConfig.ID = "MidEQ";
Engine.addModuleStateToUserPreset(eqConfig);

eqConfig.ID = "SideEQ";
Engine.addModuleStateToUserPreset(eqConfig);
```
```json:testMetadata:selective-eq-state-persistence
{
  "testable": false,
  "skipReason": "Requires EQ modules (MasterEQ, MidEQ, SideEQ) in the module tree. Builder API cannot create ParametricEQ modules."
}
```

**Pitfalls:**
- Re-registering the same module ID silently replaces the previous registration. The config object is reused here by changing its `ID` property before each call -- this is a valid pattern since each call captures the current state of the object.
