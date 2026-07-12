## toggleMidiInput

**Examples:**

```javascript:auto-enable-midi-inputs
// Title: Auto-enable all MIDI inputs in standalone mode
// Context: After a standalone installer completes, all detected MIDI
// input devices are enabled so the user can play immediately without
// manual configuration.

if(!Engine.isPlugin())
{
    for(m in Settings.getMidiInputDevices())
        Settings.toggleMidiInput(m, true);
}
```

```json:testMetadata:auto-enable-midi-inputs
{
  "testable": false,
  "skipReason": "Requires physical MIDI input devices connected to the system"
}
```
