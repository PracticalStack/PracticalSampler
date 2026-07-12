Processes audio data through the network's node graph. The `data` parameter is an array of `Buffer` objects, one per channel. All buffers must have the same sample count.

When hosting a DspNetwork in any of the five DspNetwork-capable modules (Script FX, Polyphonic Script FX, Scriptnode Synthesiser, Script Time Variant Modulator, Script Envelope Modulator), the hosting processor routes audio automatically. Leave this callback empty in hosted contexts.

> [!Warning:Silent no-output without initialisation] If the network has not been initialised via `prepareToPlay()`, processing is silently skipped. No error is thrown - the buffers are left unmodified.
