## createMessageHolder

**Examples:**

```javascript:message-holder-render-audio
// Title: Constructing MIDI events for offline audio rendering
// Context: MessageHolder objects are used to build MIDI event
// sequences for Engine.renderAudio(). Each event needs its type,
// note number, velocity, channel, and timestamp set explicitly.

inline function renderNoteToBuffer(noteNumber, lengthInSamples)
{
    local noteOn = Engine.createMessageHolder();
    local noteOff = Engine.createMessageHolder();

    noteOn.setType(noteOn.cycleId);
    noteOn.setNoteNumber(noteNumber);
    noteOn.setVelocity(100);
    noteOn.setChannel(1);
    noteOn.setTimestamp(0);

    noteOff.setType(noteOff.cycleId);
    noteOff.setNoteNumber(noteNumber);
    noteOff.setVelocity(0);
    noteOff.setChannel(1);
    noteOff.setTimestamp(lengthInSamples);

    local events = [noteOn, noteOff];

    Engine.renderAudio(events, function(status)
    {
        if (status.finished)
            Console.print("Rendered " + status.channels.length + " channels");
    });
}

// --- test-only ---
// Verify MessageHolder construction (renderAudio is async so not verified here)
const var testHolder = Engine.createMessageHolder();
testHolder.setNoteNumber(60);
testHolder.setVelocity(100);
testHolder.setChannel(1);
// --- end test-only ---
```
```json:testMetadata:message-holder-render-audio
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "testHolder.getNoteNumber()", "value": 60},
    {"type": "REPL", "expression": "testHolder.getVelocity()", "value": 100},
    {"type": "REPL", "expression": "testHolder.getChannel()", "value": 1}
  ]
}
```
