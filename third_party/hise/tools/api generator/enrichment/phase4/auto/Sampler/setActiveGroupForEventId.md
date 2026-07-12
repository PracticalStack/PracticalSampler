Sets the active round-robin group for a specific event, guaranteeing that the voice allocator uses the exact group index you set during the MIDI callback. Pass `-1` for the global group state. When using a specific event ID, this must be called from the onNoteOn callback.

Use this instead of `Sampler.setActiveGroup()` when playing multiple notes within a few milliseconds of each other - without per-event binding, the internal processing order can override the group index before voices start.

> [!Warning:Event queue cleared after each callback] The internal event queue has a limited lifetime and is cleared after each audio render callback. This method does not work with events delayed via `Message.delayEvent()` after the call.
