## setAutomationHandlerConsumesControllerEvents

**Examples:**

```javascript:enable-cc-automation-playback
// Title: Enabling CC automation playback in a step sequencer
// Context: When a MIDI sequence contains embedded CC messages (e.g. for
// filter cutoff, send levels, pitch), enable this so the automation
// handler routes them to the mapped parameters during playback.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");

// Enable CC routing to the global automation handler
mp.setAutomationHandlerConsumesControllerEvents(true);

// Now CC messages in the MIDI sequence will be sent to the
// MidiAutomationHandler, which routes them to mapped parameters.
// The CC events are consumed (not passed through the signal chain).
```
```json:testMetadata:enable-cc-automation-playback
{
  "testable": false,
  "skipReason": "Effect is only observable during playback with CC-mapped parameters; no scriptable verification"
}
```

This is essential for step sequencers that embed per-step parameter automation (e.g. filter sweeps, pitch modulation, send levels) as CC messages alongside note data. Without this, CC events in the MIDI file would be ignored by the automation system.
