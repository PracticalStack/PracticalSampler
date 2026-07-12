## clear

**Examples:**

```javascript:clearing-fx-slot-rack
// Title: Clearing an FX slot in a user-selectable effect rack
// Context: When the user selects "Off" or closes an effect panel,
// the slot needs to return to unity-gain passthrough.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SlotFX, "EffectSlot1", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SlotFX, "EffectSlot2", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SlotFX, "EffectSlot3", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SlotFX, "EffectSlot4", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var NUM_SLOTS = 4;
const var slots = [];

for (i = 0; i < NUM_SLOTS; i++)
    slots[i] = Synth.getSlotFX("EffectSlot" + (i + 1));

// Clear a specific slot back to passthrough
inline function clearSlot(slotIndex)
{
    slots[slotIndex].clear();

    // Any Effect handle from a previous setEffect() call is now invalid
}

// --- test-only ---
slots[0].setEffect("SimpleReverb");
reg preId = slots[0].getCurrentEffectId();
// --- end test-only ---

clearSlot(0);

// --- test-only ---
reg postId = slots[0].getCurrentEffectId();
// --- end test-only ---
```
```json:testMetadata:clearing-fx-slot-rack
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "preId", "value": "SimpleReverb"},
    {"type": "REPL", "expression": "postId != preId", "value": true}
  ]
}
```

**Pitfalls:**
- In practice, `setEffect("EmptyFX")` achieves the same result as `clear()` and is often preferred in switch/case selection logic because it eliminates a special case for the "off" selection. Both load the EmptyFX placeholder and enable the internal fast-path that skips all processing.
