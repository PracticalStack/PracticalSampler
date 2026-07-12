## deferCallbacks

**Examples:**

```javascript:deferred-ui-controller
// Title: Deferred callbacks for a sampler-based UI controller
// Context: A sampler plugin's main interface script handles only UI interactions
// and parameter routing -- it does not modify MIDI events. Deferring callbacks
// moves all MIDI processing to the message thread, allowing safe string
// operations, UI updates, and allocations in callbacks.

// First line of onInit -- before any other code
Synth.deferCallbacks(true);

// With deferred callbacks, you can safely do things like:
const var sampler = Synth.getSampler("MainSampler");
const var filter = Synth.getEffect("Filter");
const var reverb = Synth.getEffect("Reverb");

// Control callbacks can perform UI-safe operations
inline function onFilterControl(component, value)
{
    // String operations are safe in deferred mode
    Console.print("Filter cutoff: " + value);
    filter.setAttribute(filter.Frequency, value);
}
```
```json:testMetadata:deferred-ui-controller
{
  "testable": false,
  "skipReason": "Requires a module tree with MainSampler, Filter, and Reverb processors present"
}
```

```javascript:non-deferred-midi-processing
// Title: Non-deferred mode (default) for real-time MIDI processing
// Context: A legato script or arpeggiator must modify incoming MIDI events
// in real time. The default non-deferred mode runs callbacks on the audio
// thread with sample-accurate timing.

// Explicitly set to false (this is the default, but makes intent clear)
Synth.deferCallbacks(false);

// In non-deferred mode, Message modification works:
inline function onNoteOn()
{
    if (Synth.isLegatoInterval())
        Message.ignoreEvent(true);
}
```
```json:testMetadata:non-deferred-midi-processing
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context for onNoteOn and Message.ignoreEvent"
}
```

Use `deferCallbacks(true)` when your script is purely a UI controller for effects and samplers. Use the default `deferCallbacks(false)` when the script needs to modify MIDI events, generate artificial notes, or use sample-accurate timers.
