Returns a typed scripting wrapper for the module at the given build index. This is the Builder's equivalent of `Synth.getEffect()` or `Synth.getModulator()`, addressed by build index instead of name.

The `interfaceType` parameter selects which wrapper to return, using values from `b.InterfaceTypes`:

| Interface type | Wrapper | Typical use |
|----------------|---------|-------------|
| `"RoutingMatrix"` | ScriptRoutingMatrix | Multi-channel audio routing |
| `"SlotFX"` | ScriptingSlotFX | Loading DSP networks into HardcodedFX |
| `"Sampler"` | Sampler | Loading sample maps |
| `"Effect"` | ScriptingEffect | Bypass control, attribute access |
| `"Modulator"` | ScriptingModulator | Matrix properties, intensity |
| `"MidiProcessor"` | ScriptingMidiProcessor | MIDI processor control |
| `"ChildSynth"` | ScriptingSynth | Sound generator control |
| `"AudioSampleProcessor"` | ScriptingAudioSampleProcessor | Audio file loading |
| `"TableProcessor"` | ScriptingTableProcessor | Table curve data |
| `"SliderPackProcessor"` | ScriptSliderPackProcessor | Slider pack data |
| `"MidiPlayer"` | ScriptedMidiPlayer | MIDI sequence playback |

You can call `get()` multiple times on the same build index with different interface types to access multiple interfaces of a single module.

> [!Warning:SlotFX requires its own interface type] For HardcodedFX modules, use `b.InterfaceTypes.SlotFX` (not `Effect`) when you need to call `setEffect()` to load a DSP network. Use `Effect` for generic operations like `setBypassed()`. The two interfaces serve different purposes on the same module.
