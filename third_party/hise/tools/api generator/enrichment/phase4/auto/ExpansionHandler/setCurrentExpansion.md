Sets the currently active expansion by name (or by `Expansion` object reference) and triggers the expansion callback. Pass an empty string to clear the current expansion - the callback receives `undefined`.

While only one expansion can be "active" at a time, this does not mean you cannot load content from multiple expansions simultaneously. You do not need to call `setCurrentExpansion()` before loading data from an expansion. The active expansion is primarily a UI convenience - it allows you to adapt the interface to whichever expansion was most recently selected.

When the first expansion is activated (transitioning from no active expansion), the handler saves the current default state for later restoration.
