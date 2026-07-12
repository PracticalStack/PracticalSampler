Removes the first OSC callback registered for the given sub-address. Returns `true` if a callback was found and removed, `false` if no callback matches.

> [!Warning:Only removes the first matching callback] If multiple callbacks were registered for the same sub-address, each call removes only one. Call repeatedly until it returns `false` to remove all of them.
