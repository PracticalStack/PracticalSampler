Marks the current event so that downstream processors skip it. The event remains in the buffer but is not processed. Pass a truthy value to ignore, or a falsy value to re-enable the event.

This is the standard way to consume MIDI events in scripts - for example, suppressing keyswitch notes that should trigger articulation changes rather than producing sound, or filtering notes outside a desired range.

> [!Warning:Re-enabling may not work in deferred mode] Once an event is ignored, re-enabling it with `ignoreEvent(false)` only works if downstream processors have not already skipped it. In deferred callback mode, ignored events are skipped before the deferred callback runs, so re-enabling has no effect.
