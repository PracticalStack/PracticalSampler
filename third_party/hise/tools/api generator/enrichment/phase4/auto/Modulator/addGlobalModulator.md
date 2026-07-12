Creates a time-variant global modulator receiver in one of this module's internal chains and connects it to a source modulator inside a GlobalModulatorContainer. Returns a Modulator handle for the newly created receiver that you can control with `setIntensity`. Use `addStaticGlobalModulator` instead when the source is a voice-start type (velocity, key number, random) that only produces a value at note-on.

> [!Warning:Source must be in GlobalModulatorContainer] The `globalMod` parameter must reference a modulator inside a GlobalModulatorContainer. Passing a regular modulator fails silently.
