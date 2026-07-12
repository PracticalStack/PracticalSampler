Initialises the network for audio processing at the given sample rate and block size. Must be called before `processBlock()`. If `sampleRate` is zero or negative, the method returns without doing anything.

When hosting a DspNetwork in any of the five DspNetwork-capable modules (Script FX, Polyphonic Script FX, Scriptnode Synthesiser, Script Time Variant Modulator, Script Envelope Modulator), the hosting processor calls this automatically. Manual calls are only needed when processing buffers outside a hosted context.
