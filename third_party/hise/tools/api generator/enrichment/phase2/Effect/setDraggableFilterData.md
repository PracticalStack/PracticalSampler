## setDraggableFilterData

**Examples:**

```javascript:channel-eq-filter-config
// Title: Configure a 4-band parametric EQ with interactive drag and FFT overlay
// Context: Set up draggable filter bands on a channel EQ, then configure
// the FFT display buffer for real-time spectrum analysis behind the filter curve.

const var NUM_CHANNELS = 4;
const var channelEQs = [];

for (i = 0; i < NUM_CHANNELS; i++)
    channelEQs.push(Synth.getEffect("ChannelEq " + (i + 1)));

// Configure filter visualization for all channel EQs
const var filterConfig = {
    "NumFilterBands": 4,
    "FilterDataSlot": 0,
    "FirstBandOffset": 0,
    "TypeList": ["Low Pass", "High Pass", "Low Shelf", "High Shelf", "Peak"],
    "ParameterOrder": ["Gain", "Freq", "Q", "Enabled", "Type"],
    "FFTDisplayBufferIndex": 0,
    "DragActions": {
        "DragX": "Freq",
        "DragY": "Gain",
        "ShiftDrag": "Q",
        "DoubleClick": "Enabled",
        "RightClick": ""
    }
};

// Apply the same filter config to every channel EQ
const var fftBuffers = [];

for (eq in channelEQs)
{
    eq.setDraggableFilterData(filterConfig);

    // Get the FFT display buffer for spectrum overlay
    local dp = Synth.getDisplayBufferSource(eq.getId());
    fftBuffers.push(dp.getDisplayBuffer(0));
}

// Configure FFT display properties
for (buf in fftBuffers)
{
    buf.setRingBufferProperties({
        "BufferLength": 2048,
        "WindowType": "Flat Top",
        "DecibelRange": [-80.0, -6.0],
        "UsePeakDecay": false,
        "UseDecibelScale": true,
        "YGamma": 0.6,
        "Decay": 0.65,
        "UseLogarithmicFreqAxis": true
    });
}
```
```json:testMetadata:channel-eq-filter-config
{
  "testable": false,
  "skipReason": "Requires effects implementing ProcessorWithCustomFilterStatistics and display buffer sources; setDraggableFilterData silently does nothing on standard effects"
}
```

The `DragActions` object maps mouse gestures to filter parameters by name (matching entries in `ParameterOrder`). `DoubleClick` toggles the named parameter between 0 and 1, making it ideal for an "Enabled" toggle. Set `RightClick` to an empty string to disable right-click actions.

The `FFTDisplayBufferIndex` references a display buffer slot on the effect. Set it to -1 to disable the FFT overlay. When enabled, only activate the FFT buffer for the currently visible channel to avoid unnecessary CPU usage.
