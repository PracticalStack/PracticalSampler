## setEffect

**Examples:**

```javascript:data-driven-fx-rack
// Title: Data-driven FX rack with configurable effect slots
// Context: An FX rack where users select effects from a popup menu.
// Each effect type is defined in a configuration object that maps
// the UI name to a module ID, parameter bindings, and a default state.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SlotFX, "EffectSlot1", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SlotFX, "EffectSlot2", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SlotFX, "EffectSlot3", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SlotFX, "EffectSlot4", 0, builder.ChainIndexes.FX);
builder.flush();
Content.addKnob("FxKnob1", 0, 0);
Content.addKnob("FxKnob2", 0, 0);
Content.addKnob("FxKnob3", 0, 0);
Content.addKnob("FxKnob4", 0, 0);
// --- end setup ---

// Effect configuration: maps each effect to its module type and parameters
const var FX_CONFIG = {
    "chorus": {
        "moduleID": "Chorus",
        "parameterIds": ["Rate", "Width", "Feedback", "Delay"],
        "ranges": [[0, 1, 0.3], [0, 1, 0.43], [0, 1, 0.3], [0, 1, 1.0]]
    },
    "reverb": {
        "moduleID": "SimpleReverb",
        "parameterIds": ["WetLevel", "Damping", "Width", "RoomSize"],
        "ranges": [[0, 1, 0.2], [0, 1, 0.6], [0, 1, 0.8], [0, 1, 0.8]]
    },
    "delay": {
        "moduleID": "Delay",
        "parameterIds": ["DelayTimeLeft", "FeedbackLeft", "Mix"],
        "ranges": [[0, 1000, 200], [0, 1, 0.3], [0, 1, 0.5]]
    }
};

const var NUM_SLOTS = 4;
const var slots = [];
const var knobs = [];

// Get SlotFX references and knob arrays at init
for (i = 0; i < NUM_SLOTS; i++)
{
    slots[i] = Synth.getSlotFX("EffectSlot" + (i + 1));
    knobs[i] = Content.getComponent("FxKnob" + (i + 1));
}

// Load an effect into a slot by config key
inline function loadEffect(slotIndex, fxKey)
{
    if (fxKey == "empty")
    {
        slots[slotIndex].setEffect("EmptyFX");
        return;
    }

    local config = FX_CONFIG[fxKey];

    // setEffect returns the loaded Effect handle
    local fx = slots[slotIndex].setEffect(config.moduleID);

    // Bind UI knobs to the loaded effect's parameters
    local slotName = fx.getId();

    for (j = 0; j < config.parameterIds.length; j++)
    {
        knobs[slotIndex].set("processorId", slotName);
        knobs[slotIndex].set("parameterId", config.parameterIds[j]);
    }
}

loadEffect(0, "chorus");
loadEffect(1, "reverb");
```
```json:testMetadata:data-driven-fx-rack
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "slots[0].getCurrentEffectId()", "value": "Chorus"},
    {"type": "REPL", "expression": "slots[1].getCurrentEffectId()", "value": "SimpleReverb"}
  ]
}
```

```javascript:builder-hardcoded-master-fx
// Title: Builder API with compiled DSP networks in HardcodedMasterFX
// Context: A multi-channel instrument that programmatically builds its
// signal chain, creating HardcodedMasterFX modules and loading compiled
// scriptnode networks into them through the SlotFX interface.

const var b = Synth.createBuilder();
b.clear();

// Create a channel strip with EQ, compressor, and filter
inline function buildChannelStrip(channelIndex)
{
    local eq = b.create(
        b.Effects.HardcodedMasterFX,
        "ChannelEq " + (channelIndex + 1),
        0,
        b.ChainIndexes.FX
    );

    // Load a compiled DSP network into the HardcodedMasterFX slot
    b.get(eq, b.InterfaceTypes.SlotFX).setEffect("fourband_eq");

    // Start bypassed -- user enables per preset
    b.get(eq, b.InterfaceTypes.Effect).setBypassed(true);

    local comp = b.create(
        b.Effects.HardcodedMasterFX,
        "ChannelComp " + (channelIndex + 1),
        0,
        b.ChainIndexes.FX
    );

    b.get(comp, b.InterfaceTypes.SlotFX).setEffect("compressor");
    b.get(comp, b.InterfaceTypes.Effect).setBypassed(true);
}

buildChannelStrip(0);
buildChannelStrip(1);
b.flush();
```
```json:testMetadata:builder-hardcoded-master-fx
{
  "testable": false,
  "skipReason": "Requires compiled scriptnode networks (fourband_eq, compressor) in the project"
}
```

**Pitfalls:**
- When managing multiple SlotFX instances in an array, avoid looking up the loaded effect by its internal child name (e.g., `Synth.getEffect("SlotName_EffectType")`). The internal naming convention is an implementation detail. Always use the `Effect` handle returned directly by `setEffect()`.
