<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# ComplexGroupManager

ComplexGroupManager provides multi-dimensional control over sample group filtering for a Sampler. Instead of the Sampler's single round-robin group index, it organises samples into independent layers, each with its own filtering logic. A 64-bit bitmask encodes every sample's membership across all layers, so multiple dimensions can be evaluated simultaneously during voice start.

```js
const var cgm = Sampler1.getComplexGroupManager();
```

Each layer has a LogicType that determines its automatic behaviour:

| LogicType | Purpose |
|-----------|---------|
| Custom | Scriptable layer with per-group volume and voice start callbacks |
| RoundRobin | Automatic round-robin cycling on each note |
| Keyswitch | MIDI-key-driven articulation switching |
| TableFade | Legacy crossfade using the Sampler's crossfade tables |
| XFade | Dynamic crossfade from EventData, MIDI CC, or GlobalMod sources |
| LegatoInterval | Phase-locked legato transition samples |
| ReleaseTrigger | Automatic release sample triggering with gain matching |
| Choke | Mutual voice cancellation between groups |

Layers are configured through string property IDs via `getLayerProperty()` and `setLayerProperty()`. Voice start parameters (delay, fade-in, start offset, fixed length) are set per layer/group before a noteOn and consumed when voices start.

The constant `ComplexGroupManager.IgnoreFlag` (255) indicates that a layer does not apply to a given sample. Methods that accept a layer identifier take either a zero-based numeric index or a string layer ID.

> [!Tip:Zero-based group indices, next-noteOn configuration] All group indices in the scripting API are zero-based. The internal bitmask is one-based, but the conversion is automatic. Methods that modify voice start parameters (`delayGroupEvent`, `fadeInGroupEvent`, `addGroupEventStartOffset`, `setFixedGroupEventLength`) configure data for the next noteOn - they do not affect already-playing voices.

## Common Mistakes

- **setGroupVolume requires Custom LogicType**
  **Wrong:** `cgm.setGroupVolume(0, 1, 0.5);` on a non-Custom layer
  **Right:** `cgm.setGroupVolume(0, 1, 0.5);` on a Custom layer
  *`setGroupVolume` only works with Custom LogicType layers. On other layer types, the call silently does nothing.*

- **Call createNoteMap before querying**
  **Wrong:** `cgm.isNoteNumberMapped(0, 60);` without calling `createNoteMap` first
  **Right:** `cgm.createNoteMap(0); cgm.isNoteNumberMapped(0, 60);`
  *`isNoteNumberMapped` requires `createNoteMap()` to have been called first for the same layer. Call it again after loading a new sample map.*

- **Enable gain tracking before reading peaks**
  **Wrong:** `cgm.getCurrentPeak(0, 1, eventId);` without enabling tracking
  **Right:** `cgm.setEnableGainTracking(0, 1, true);` then later `cgm.getCurrentPeak(0, 1, eventId);`
  *Peak tracking must be explicitly enabled per layer/group pair before values can be read.*
