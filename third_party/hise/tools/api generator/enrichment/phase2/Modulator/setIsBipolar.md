## setIsBipolar

**Examples:**

```javascript:matrix-modulator-pitch-config
// Title: Configure a MatrixModulator for pitch modulation
// Context: When creating MatrixModulators for a modulation matrix via the
// Builder API, pitch targets need specific bipolar and intensity settings.
// Disabling bipolar ensures the modulation range starts from zero.

const var builder = Synth.createBuilder();

// Create a MatrixModulator in the pitch chain of an oscillator group
const var matrixModule = builder.create(
    builder.Modulators.MatrixModulator, "PitchMod", oscGroup, pitchChainIndex
);

const var mod = builder.get(matrixModule, builder.InterfaceTypes.Modulator);

// For pitch targets: disable bipolar so range is 0..intensity (not -intensity..+intensity)
mod.setIsBipolar(false);

// Set pitch range in semitones
mod.setIntensity(12.0);
```
```json:testMetadata:matrix-modulator-pitch-config
{
  "testable": false,
  "skipReason": "Requires Builder API context with an oscillator group and pitch chain"
}
```
