Converts the current event into an artificial event with a new event ID, even if the event is already artificial. Unlike `Message.makeArtificial()` which is idempotent and returns the existing ID for already-artificial events, this method always assigns a fresh ID.

Use this when you need to create multiple independent branches of an event, each with its own event ID for independent voice control. For simple conversion where duplicates are unwanted, use `Message.makeArtificial()` instead.

> [!Warning:Overwrites ID cache on artificial notes] Calling this on an already-artificial note-on overwrites the internal ID cache for that note number. The previous event ID is lost, which may break note-off matching if both IDs need to be tracked.
