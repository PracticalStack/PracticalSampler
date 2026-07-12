## setRingBufferProperties

**Examples:**

```javascript:shared-fft-config-multi-eq
// Title: Configure FFT spectrum analyser with shared properties
// Context: A plugin with multiple EQ modules (e.g., per-mic or M/S processing)
// shares one FFT configuration object across all display buffers for consistency.

const var FFT_PROPERTIES = {
    "BufferLength": 4096,
    "WindowType": "Flat Top",
    "DecibelRange": [-80.0, -6.0],
    "UsePeakDecay": false,
    "UseDecibelScale": true,
    "YGamma": 0.6,
    "Decay": 0.65,
    "UseLogarithmicFreqAxis": true
};

// Apply identical settings to all EQ analyser buffers
const var eqNames = ["MasterEQ", "MidEQ", "SideEQ"];

for (name in eqNames)
{
    local db = Synth.getDisplayBufferSource(name).getDisplayBuffer(0);
    db.setRingBufferProperties(FFT_PROPERTIES);
}
```
```json:testMetadata:shared-fft-config-multi-eq
{
  "testable": false,
  "skipReason": "Requires MasterEQ, MidEQ, and SideEQ modules with FFT display buffer sources"
}
```

```javascript:compact-buffer-channel-eq
// Title: Configure a per-channel EQ analyser with compact buffer
// Context: Per-channel EQ analysers in a mixer-style plugin use smaller buffers
// for lower CPU cost since the display panels are smaller.

const var db = Synth.getDisplayBufferSource("ChannelEQ").getDisplayBuffer(0);

db.setRingBufferProperties({
    "BufferLength": 2048,
    "WindowType": "Flat Top",
    "DecibelRange": [-80.0, -6.0],
    "UsePeakDecay": false,
    "UseDecibelScale": true,
    "YGamma": 0.6,
    "Decay": 0.65,
    "UseLogarithmicFreqAxis": true
});
```
```json:testMetadata:compact-buffer-channel-eq
{
  "testable": false,
  "skipReason": "Requires ChannelEQ module with FFT display buffer source"
}
```

**Pitfalls:**
- Property names are case-sensitive and silently ignored if misspelled. Use the exact capitalisation: `"BufferLength"`, `"WindowType"`, `"DecibelRange"`, `"UsePeakDecay"`, `"UseDecibelScale"`, `"YGamma"`, `"Decay"`, `"UseLogarithmicFreqAxis"`. There is no error message for typos like `"bufferLength"` or `"WindowType "` (trailing space).
- The `"WindowType"` value `"Flat Top"` (with space) and `"FlatTop"` (no space) may both appear in different contexts. Verify the accepted string for the HISE version in use.
