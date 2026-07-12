## sendGridSyncOnNextCallback

**Examples:**

```javascript:resync-grid-after-preset
// Title: Resync grid after preset load for clean playback restart
// Context: After loading a new preset, the grid position may be stale. Calling
// sendGridSyncOnNextCallback() before restarting the clock ensures the next
// grid callback has firstGridInPlayback set to true, allowing sequencers to
// reset their position counters.
const var th = Engine.createTransportHandler();
th.setSyncMode(th.PreferInternal);
th.setEnableGrid(true, 8);

// After a preset finishes loading:
th.sendGridSyncOnNextCallback();

// Restart the clock with a short delay to allow preset state to settle
th.startInternalClock(0);

```
```json:testMetadata:resync-grid-after-preset
{
  "testable": false
}
```


**Pitfalls:**
- This is a global operation -- it affects all TransportHandler instances, not just the one you call it on. In a multi-instance setup, calling this from any handler resets the grid sync flag for all handlers.
