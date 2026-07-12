## getControllerNumber

**Examples:**

```javascript:unified-controller-routing
// Title: Unified controller routing with virtual CC numbers
// Context: The onController callback receives CC, pitch bend, aftertouch,
// and program change events. Use getControllerNumber() with the virtual
// CC constants to handle them uniformly.

function onController()
{
    local cc = Message.getControllerNumber();
    local val = Message.getControllerValue();

    if (cc == 64)
    {
        // Sustain pedal
        local pedalDown = val > 64;
        // ... handle sustain logic
    }
    else if (cc == 1)
    {
        // Mod wheel -> vibrato depth
        Synth.getModulator("Vibrato").setIntensity(val / 127.0);
    }
    else if (cc == Message.PITCH_BEND_CC) // 128
    {
        // Pitch wheel: val is 0-16383 (14-bit), not 0-127
        local normalized = (val - 8192) / 8192.0; // -1.0 to +1.0
        // ... apply pitch bend
    }
    else if (cc == Message.AFTERTOUC_CC) // 129
    {
        // Channel aftertouch: val is 0-127
        // ... apply aftertouch modulation
    }
    else if (Message.isProgramChange())
    {
        // Program change events also route through onController
        local pgm = Message.getProgramChangeNumber();
        // ... switch preset
    }
}
```
```json:testMetadata:unified-controller-routing
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```
