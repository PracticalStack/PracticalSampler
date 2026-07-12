Creates or retrieves a scriptnode DSP network with the given ID. If a network with the same ID already exists on this processor, it is returned without creating a duplicate. The script processor must implement the `DspNetwork::Holder` interface.

The following modules support `createDspNetwork`: Script FX (`ScriptFX`), Polyphonic Script FX (`PolyScriptFX`), Scriptnode Synthesiser (`ScriptSynth`), Script Time Variant Modulator (`ScriptTimeVariantModulator`), and Script Envelope Modulator (`ScriptEnvelopeModulator`).

> [!Warning:Only DspNetwork::Holder modules supported] Calling this on a plain Script Processor or Script Voice Start Modulator produces a script error. Only the five module types listed above can create DSP networks.