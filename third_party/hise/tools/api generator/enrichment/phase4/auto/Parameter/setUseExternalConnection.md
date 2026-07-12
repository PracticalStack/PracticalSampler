Marks this parameter as externally controlled. This is analogous to how a scriptnode modulation source or container parameter connection works internally - the script becomes the source of truth for this parameter's value.

After calling `setUseExternalConnection(true)`:
- `setValue()` applies values directly to the node without storing them
- The persistent value is cleared (it will be restored when you switch back)
- `setValue()` becomes safe to call from the audio thread

Call this once during setup before using `setValue()` in a timer, processBlock callback, or any other audio-thread context.

```js
const var p = nd.getOrCreateParameter("Cutoff");
p.setUseExternalConnection(true);

// Now setValue() applies directly - safe for audio-thread use
p.setValue(0.5);
```

To release external control and return to persistent mode, call `setUseExternalConnection(false)`. The parameter restores its value from the current state or its default.

> [!Warning:Value jump on mode switch] When switching back to `false`, the restored value may differ from the last value set externally. This can produce an audible jump if the parameter is driving a continuous process.
