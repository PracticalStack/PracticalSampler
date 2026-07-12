Enables or disables the release start feature for a specific event or globally. Pass `-1` as the event ID to toggle the feature for all events; pass a valid event ID to control it per-note (e.g. disabling release skip for legato intervals). Returns `true` if the event ID was found.

It is safe and recommended to call this in the onNoteOff callback - the setting is picked up correctly by the sampler before the release phase begins.
