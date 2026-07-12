## removeConnection

**Examples:**

```javascript:mid-side-channel-isolation
// Title: M/S processing -- isolating mid and side channels
// Context: In a mid/side processing chain, two EQ processors sit between
// LR-to-MS and MS-to-LR converter effects. Each EQ is restricted to one
// channel by removing the other connection from its routing matrix.

const var b = Synth.createBuilder();

// LR-to-MS encoder
local lr2ms = b.create(b.Effects.HardcodedMasterFX, "LR2MS", 0, b.ChainIndexes.FX);

// Mid-only EQ: remove the side channel (channel 1)
local midEq = b.create(b.Effects.CurveEq, "MidEQ", 0, b.ChainIndexes.FX);
b.get(midEq, b.InterfaceTypes.RoutingMatrix).removeConnection(1, 1);

// Side-only EQ: remove the mid channel (channel 0)
local sideEq = b.create(b.Effects.CurveEq, "SideEQ", 0, b.ChainIndexes.FX);
b.get(sideEq, b.InterfaceTypes.RoutingMatrix).removeConnection(0, 0);

// MS-to-LR decoder
local ms2lr = b.create(b.Effects.HardcodedMasterFX, "MS2LR", 0, b.ChainIndexes.FX);

b.flush();
```
```json:testMetadata:mid-side-channel-isolation
{
  "testable": false,
  "skipReason": "Requires HardcodedMasterFX module type (LR2MS/MS2LR converters) which depends on project DLL availability"
}
```

Using `removeConnection` on individual channels is more precise than `clear()` when you want to keep most of the default routing intact and only disable specific signal paths. This is particularly useful for mid/side or multi-band processing where each processor handles a subset of channels.
