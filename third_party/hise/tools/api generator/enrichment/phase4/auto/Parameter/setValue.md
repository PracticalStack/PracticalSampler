Sets the parameter to a new value. The behaviour depends on whether the parameter is in default mode or has been marked as externally controlled via `setUseExternalConnection()`.

**Default mode** - the value is stored persistently and supports undo. The node picks up the new value automatically. Use this for UI changes, preset recall, and programmatic setup:

```js
const var p = nd.getOrCreateParameter("Volume");
p.setValue(0.75);  // stored persistently, supports undo
```

**External connection mode** - the value is applied directly to the node without being stored. Use this when your script continuously drives the parameter (timers, MIDI CC, audio-thread callbacks). Call `setUseExternalConnection(true)` once before using this mode:

```js
const var p = nd.getOrCreateParameter("Cutoff");
p.setUseExternalConnection(true);
p.setValue(0.5);  // applied directly, not stored
```

This replaces the deprecated `setValueAsync()` and `setValueSync()` methods. Configure the mode once with `setUseExternalConnection()`, then use `setValue()` throughout.

> [!Warning:Audio-thread safety] If you call `setValue()` from the audio thread (e.g. inside a timer or processBlock callback), you must call `setUseExternalConnection(true)` first. Without it, `setValue()` attempts to write persistent state from the audio thread, which triggers a script error in the HISE IDE. In exported plugins this check is absent, risking thread-safety issues.
