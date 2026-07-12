Returns the value of a layer configuration property. The return type depends on the property - most return a String or Number, but `tokens` returns a String array and `matrixString` returns an integer array.

| Property | Description |
|----------|-------------|
| `"type"` | LogicType name (`"Custom"`, `"RR"`, `"Keyswitch"`, `"TableFade"`, `"XFade"`, `"Legato"`, `"Release"`, `"Choke"`) |
| `"id"` | Layer identifier string |
| `"tokens"` | Group names within the layer (String array) |
| `"colour"` | Display colour |
| `"folded"` | UI folded/collapsed state |
| `"ignorable"` | Whether samples in this layer can use IgnoreFlag |
| `"cached"` | Whether samples are pre-filtered into per-group containers |
| `"purgable"` | Whether samples can be purged by group value |
| `"fader"` | XFade curve type (`"Linear"`, `"RMS"`, `"Cosine half"`, `"Overlap"`, `"Switch"`) |
| `"slotIndex"` | XFade data slot index or MIDI CC number |
| `"sourceType"` | XFade input source (`"Event Data"`, `"Midi CC"`, `"GlobalMod"`) |
| `"matrixString"` | Choke group relationship matrix (integer array) |
| `"isChromatic"` | Whether keyswitch layer includes black keys |
| `"matchGain"` | Whether release layer matches sustain peak on release |
| `"accuracy"` | Release gain matching accuracy (0.0-1.0) |
| `"fadeTime"` | Fade duration in milliseconds |