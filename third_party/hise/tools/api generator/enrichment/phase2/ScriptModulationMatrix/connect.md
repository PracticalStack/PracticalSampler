## connect

**Examples:**

```javascript:programmatic-connection-setup
// Title: Programmatic connection setup with property configuration
// Context: A synth initializes default modulation routing on first load,
// connecting an envelope to gain and an LFO to filter cutoff with
// specific intensity and mode settings.

const var mm = Engine.createModulationMatrix("Global Modulator Container0");

// Check availability before connecting
if (mm.canConnect("AHDSR", "Gain"))
    mm.connect("AHDSR", "Gain", true);

if (mm.canConnect("LFO", "Filter Frequency"))
{
    mm.connect("LFO", "Filter Frequency", true);

    // Configure the connection properties after creation
    mm.setConnectionProperty("LFO", "Filter Frequency", "Intensity", 0.5);
    mm.setConnectionProperty("LFO", "Filter Frequency", "Mode", 1); // Unipolar
}

// Remove a connection programmatically
mm.connect("AHDSR", "Gain", false);
```

```json:testMetadata:programmatic-connection-setup
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer with AHDSR, LFO sources and Gain, Filter Frequency targets in the module tree"
}
```
