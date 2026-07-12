Registers a callback that fires when the audio content changes - whether from loading a file, modifying the buffer, or changing the range. The callback takes no parameters; use `this` inside the callback body to access the AudioFile that changed (e.g. `this.getCurrentlyLoadedFile()`, `this.getNumSamples()`).

> [!Warning:Callback fires asynchronously] The callback fires asynchronously. Code that runs immediately after `loadFile()` executes before the callback has fired.
