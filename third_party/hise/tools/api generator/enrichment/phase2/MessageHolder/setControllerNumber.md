## setControllerNumber

**Examples:**

```javascript:construct-cc-and-pitchbend-events
// Title: Constructing CC and pitch bend events for a MIDI sequence
// Context: A sequencer stores both note and controller data. CC events
// and pitch bend events are both created through the controller API,
// using virtual CC number 128 for pitch bend type coercion.

const var PITCH_BEND_CC = 128;

inline function addControllerEvent(eventList, ccNumber, value, startTick)
{
    local cc = Engine.createMessageHolder();

    // Virtual CC 128 sets type to PitchBend automatically
    if (ccNumber == PITCH_BEND_CC)
        cc.setType(cc.PitchBend);
    else
        cc.setType(cc.Controller);

    cc.setChannel(1);
    cc.setControllerNumber(ccNumber);

    local intValue = Math.range(Math.floor(value * 128.0), 0, 127);
    cc.setControllerValue(intValue);
    cc.setTimestamp(Math.max(0, startTick));

    eventList.push(cc);
}

// Usage: add a mod wheel CC and a pitch bend to a sequence
var events = [];
addControllerEvent(events, 1, 0.5, 0);      // CC#1 (mod wheel) = 64
addControllerEvent(events, PITCH_BEND_CC, 0.5, 240);  // Pitch bend = 64

Console.print(events[0].dump());
Console.print(events[1].dump());
```
```json:testMetadata:construct-cc-and-pitchbend-events
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Type: Controller, Channel: 1, Number: 1, Value: 64, EventId: 0, Timestamp: 0, ", "Type: PitchBend, Channel: 1, Value: 64, Timestamp: 240, "]}
  ]
}
```

Note that `setControllerNumber(128)` changes the event type to PitchBend as a side effect. If you already set the type to Controller before calling `setControllerNumber(128)`, the type will be silently overwritten to PitchBend. The reverse direction is consistent: `getControllerNumber()` returns 128 for PitchBend events and 129 for Aftertouch events.
