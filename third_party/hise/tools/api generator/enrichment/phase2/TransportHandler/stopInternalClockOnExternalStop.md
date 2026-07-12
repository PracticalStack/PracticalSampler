## stopInternalClockOnExternalStop

**Examples:**

```javascript:standard-transport-setup-with
// Title: Standard transport setup with automatic stop on DAW stop
// Context: When using PreferInternal mode with a host-sync option, enabling this
// setting ensures the internal clock stops cleanly when the DAW transport stops.
// Without it, stopping the DAW while host-synced leaves the internal clock in
// an ambiguous state.
const var th = Engine.createTransportHandler();

// Standard setup sequence for a plugin with internal transport + host sync
th.setEnableGrid(true, 8);                    // 1/8 note grid
th.setSyncMode(th.PreferInternal);            // Internal clock by default
th.stopInternalClockOnExternalStop(true);     // Clean stop when DAW stops

```
```json:testMetadata:standard-transport-setup-with
{
  "testable": false
}
```

