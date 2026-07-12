Removes a previously added effect from the parent synth's effect chain. Pass the `ScriptEffect` handle returned by `Synth.addEffect()` or `Synth.getEffect()`. The removal is asynchronous - the effect may still be briefly active after the call returns.

> [!Warning:Not audio-thread safe] Cannot be called from the audio thread. Use deferred callbacks or call from `onInit` / `onControl`.
