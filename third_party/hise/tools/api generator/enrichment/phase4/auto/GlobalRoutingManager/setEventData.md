Stores a double value in the per-event data storage, keyed by MIDI event ID and slot index (0-15). The stored value persists with the event and can be retrieved later in the processing chain by `getEventData`, the EventData Modulator, or the `routing.event_data_reader` scriptnode node.

This is safe to call from MIDI callbacks - the storage uses a fixed-size lookup table with no allocations or locks.

> [!Warning:Hash collisions with concurrent events] The storage uses bitmask hashing (1024 event slots). Events whose IDs share the same lower 10 bits map to the same slot, silently overwriting the previous entry. In practice this is rare since MIDI event IDs cycle sequentially, but it can occur under heavy polyphony with artificial event IDs.
