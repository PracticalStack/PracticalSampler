Reads a value from the per-event data storage for the given MIDI event ID and slot index (0-15). Returns the stored double value on success, or `undefined` if the slot was never written or if a hash collision has overwritten the entry.

Pair with `setEventData` to attach and retrieve custom numeric data on MIDI events. The same data is also readable downstream by the EventData Modulator and the `routing.event_data_reader` scriptnode node.
