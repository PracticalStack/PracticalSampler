Registers a callback that fires when sample preloading starts or finishes. The callback receives a boolean: `true` when loading begins, `false` when it completes. Pass a non-function value to remove the listener.

A common pattern is to start a timer-driven spinner or progress bar when loading begins and stop it when loading completes, using `Engine.getPreloadProgress()` to poll progress.

> [!Warning:Use Broadcaster for samplemap event order] For data logic that depends on the correct execution order of samplemap events, use `Broadcaster.attachToSampleMap()` instead - it provides finer-grained control.
