## attachToModuleParameter

**Examples:**

```javascript:eq-vca-multi-processor
// Title: EQ Virtual Control Array - one set of knobs controls multiple EQ processors
// Context: When a plugin has multiple EQ processors (e.g., master/mid/side in a
// stereo FX chain), a single broadcaster watching all processors keeps shared VCA
// knobs in sync with whichever processor is currently active.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.CurveEq, "MasterEQ", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.CurveEq, "MidEQ", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.CurveEq, "SideEQ", 0, builder.ChainIndexes.FX);
builder.flush();
const var EQFreqK = Content.addKnob("EQFreqK", 0, 0);
EQFreqK.set("saveInPreset", false);
const var EQGainK = Content.addKnob("EQGainK", 150, 0);
EQGainK.set("saveInPreset", false);
const var EQQK = Content.addKnob("EQQK", 300, 0);
EQQK.set("saveInPreset", false);
// --- end setup ---

const var EQs = [Synth.getEffect("MasterEQ"),
                 Synth.getEffect("MidEQ"),
                 Synth.getEffect("SideEQ")];

const var vcaKnobs = [Content.getComponent("EQGainK"),
                      Content.getComponent("EQFreqK"),
                      Content.getComponent("EQQK")];

// Build parameter index list: 4 bands x 5 params each = 20 parameters
const var EQ_PARAMS = [];
for (i = 0; i < 20; i++)
    EQ_PARAMS.push(i);

reg currentBand = 0;
reg currentEqIndex = 0;

const var paramWatcher = Engine.createBroadcaster({
    "id": "EQParamSync",
    "args": ["processorId", "parameterId", "value"]
});

// Watch all 20 parameters across all 3 EQ processors
paramWatcher.attachToModuleParameter(
    ["MasterEQ", "MidEQ", "SideEQ"],
    EQ_PARAMS,
    "syncVCA"
);

// Only update knobs for the active EQ and selected band
paramWatcher.addListener("", "updateVCA", function(eqId, paramIndex, value)
{
    if (EQs[currentEqIndex].getId() != eqId)
        return;

    // Check if this parameter belongs to the currently selected band
    local bandOffset = currentBand * 5;

    if (paramIndex >= bandOffset && paramIndex < bandOffset + 3)
        vcaKnobs[paramIndex - bandOffset].setValue(value);
});
```
```json:testMetadata:eq-vca-multi-processor
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "paramWatcher.processorId", "value": "SideEQ"}
  ]
}
```

```javascript:bypass-disables-controls
// Title: Greying out controls when a module is bypassed
// Context: Use the special "Bypassed" parameter ID to react to a module's
// bypass state and update the UI accordingly.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SimpleGain, "BypassTarget", 0, builder.ChainIndexes.FX);
builder.flush();
const var CompThreshold = Content.addKnob("CompThreshold", 0, 0);
CompThreshold.set("saveInPreset", false);
const var CompRatio = Content.addKnob("CompRatio", 150, 0);
CompRatio.set("saveInPreset", false);
const var CompAttack = Content.addKnob("CompAttack", 300, 0);
CompAttack.set("saveInPreset", false);
// --- end setup ---

const var disabler = Engine.createBroadcaster({
    "id": "BypassWatcher2",
    "args": ["processorId", "parameterId", "value"]
});

disabler.attachToModuleParameter("BypassTarget", "Bypassed", "bypassState");

const var compKnobs = [Content.getComponent("CompThreshold"),
                       Content.getComponent("CompRatio"),
                       Content.getComponent("CompAttack")];

// Grey out knobs when the compressor is bypassed
disabler.addComponentPropertyListener(compKnobs, "enabled",
    "disableOnBypass",
    function(targetIndex, processorId, parameterId, isBypassed)
{
    return !isBypassed;
});
```
```json:testMetadata:bypass-disables-controls
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Content.getComponent(\"CompThreshold\").get(\"enabled\")", "value": true},
    {"type": "REPL", "expression": "Content.getComponent(\"CompRatio\").get(\"enabled\")", "value": true}
  ]
}
```

**Pitfalls:**
- The `moduleIds` parameter must be string IDs, not scripting object references. Passing a reference from `Synth.getEffect()` produces an error. Use the processor's string ID instead.
