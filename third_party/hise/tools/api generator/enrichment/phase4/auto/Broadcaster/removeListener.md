Removes the first listener target whose metadata ID matches the given parameter. Pass the same metadata string or JSON object used when the listener was registered. Returns `true` if a match was found and removed, `false` otherwise.

> [!Warning:Removal requires metadata ID match] Matching is by metadata ID, not by object reference or callback function. Passing the original `object` or `function` from `addListener` does not work for removal - you must pass the metadata identifier.
