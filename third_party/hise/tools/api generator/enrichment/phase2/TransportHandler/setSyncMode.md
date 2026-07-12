## setSyncMode

**Examples:**

```javascript:host-sync-toggle-button-that
// Title: Host-sync toggle button that switches between internal and external clock
// Context: A drum machine needs its own play/stop controls (internal clock) but should
// follow the DAW when the user enables host sync. A toggle button switches between
// PreferInternal (default) and PreferExternal modes at runtime.
const var th = Engine.createTransportHandler();

// Default: internal clock takes priority
th.setSyncMode(th.PreferInternal);
th.stopInternalClockOnExternalStop(true);

const var HostSyncButton = Content.addButton("HostSync", 0, 0);
HostSyncButton.set("saveInPreset", false);

inline function onHostSync(component, value)
{
    if (value)
        th.setSyncMode(th.PreferExternal);
    else
        th.setSyncMode(th.PreferInternal);
}

HostSyncButton.setControlCallback(onHostSync);

```
```json:testMetadata:host-sync-toggle-button-that
{
  "testable": false
}
```


```javascript:connected-midi-processor-with
// Title: Connected MIDI processor with external-only vs internal-only modes
// Context: A simpler alternative for plugins that do not need fallback behavior.
// ExternalOnly ignores internal clock entirely; InternalOnly ignores the DAW.
const var th = Engine.createTransportHandler();

const var ExternalButton = Content.addButton("External", 0, 0);

inline function onExternalToggle(component, value)
{
    if (value)
        th.setSyncMode(th.ExternalOnly);
    else
        th.setSyncMode(th.InternalOnly);
}

ExternalButton.setControlCallback(onExternalToggle);

```
```json:testMetadata:connected-midi-processor-with
{
  "testable": false
}
```

