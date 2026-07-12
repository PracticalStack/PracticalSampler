Adds a sample offset to the event's timestamp, pushing it later in time within the audio buffer. If the resulting timestamp exceeds the current buffer size, the event is automatically carried over to a future buffer. The delay is additive relative to the event's existing timestamp.

> [!Warning:Delays processing, not sample playback] This delays when the event is *processed*, not where sample playback begins. To skip ahead in a sample without delaying the event, use `Message.setStartOffset()` instead.

> [!Warning:Guard with isArtificial for humanisation] When using this for humanisation, guard with `Message.isArtificial()` so that only sequencer-generated notes are delayed. Live MIDI input should pass through without added latency.
