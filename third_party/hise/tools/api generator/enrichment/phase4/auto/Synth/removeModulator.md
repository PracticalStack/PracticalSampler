Removes a previously added modulator from the parent synth's modulation chain. Pass the `ScriptModulator` handle returned by `Synth.addModulator()` or `Synth.getModulator()`. The removal is asynchronous - the modulator may still be briefly active after the call returns.

> [!Warning:Not audio-thread safe] Cannot be called from the audio thread. Use deferred callbacks or call from `onInit` / `onControl`.
