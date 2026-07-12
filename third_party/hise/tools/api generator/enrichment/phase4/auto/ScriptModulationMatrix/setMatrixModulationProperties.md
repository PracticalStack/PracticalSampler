Configures global properties for the modulation matrix. The JSON object accepts three sections:

- `SelectableSources` (bool) - enable exclusive source selection mode
- `DefaultInitValues` (object) - per-target defaults applied when new connections are created, with `Intensity`, `Mode` (`"Scale"`, `"Unipolar"`, or `"Bipolar"`), and optional `IsNormalized` keys
- `RangeProperties` (object) - per-target range configuration using either a preset name or a custom range object

Available range presets: `NormalizedPercentage`, `Gain0dB`, `Gain6dB`, `Pitch1Octave`, `Pitch2Octaves`, `Pitch1Semitone`, `PitchOctaveStep`, `PitchSemitoneStep`, `FilterFreq`, `FilterFreqLog`, `Stereo`.

The recommended workflow is to fetch the current state with `getMatrixModulationProperties()`, modify the values you need, then pass the object back.

> [!Warning:RangeProperties accepts two formats] Each entry in `RangeProperties` can be either a preset name string (e.g. `"FilterFreq"`) or a full JSON object with `InputRange`, `OutputRange`, and `mode` keys. Both formats can be mixed in the same call.
