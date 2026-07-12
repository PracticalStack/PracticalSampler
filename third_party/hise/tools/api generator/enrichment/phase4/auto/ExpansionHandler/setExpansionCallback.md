Sets a callback that fires when the active expansion changes. The callback receives a single argument: an `Expansion` object reference when an expansion is activated, or `undefined` when the current expansion is cleared.

The callback fires in these situations:

1. A preset belonging to an expansion is loaded
2. `setCurrentExpansion()` is called
3. The user selects an expansion from a dropdown connected to the expansion system

The callback does not fire at registration time. To initialise the UI for the default (no-expansion) state, call your callback function manually with `undefined` after registering it:

```javascript
eh.setExpansionCallback(onExpansionLoaded);
onExpansionLoaded(undefined);
```

> [!Warning:Callback is owned by the wrapper object] Like the error function, the expansion callback is owned by the ExpansionHandler wrapper, not the global expansion manager. Store the wrapper in a persistent `const var` in `onInit`.
