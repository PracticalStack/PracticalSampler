## setBypassed

**Examples:**

```javascript:mode-layer-matrix
// Title: Mode-based layer enabling/disabling
// Context: A piano plugin has multiple sound layers (sustain, release, pedal,
// hammer noise) that are enabled or disabled based on a performance mode.
// A mode matrix defines which layers are active in each mode.

const var layers = [Synth.getChildSynth("Sustain"),
                    Synth.getChildSynth("Release"),
                    Synth.getChildSynth("PedalNoise"),
                    Synth.getChildSynth("HammerNoise")];

//                    Sustain  Release  Pedal  Hammer
const MODE_MATRIX = [[false,   false,   false, false],   // Full
                     [false,   false,   true,  true],     // Simple
                     [false,   true,    true,  true]];    // Minimal

inline function setMode(modeIndex)
{
    for (i = 0; i < layers.length; i++)
        layers[i].setBypassed(MODE_MATRIX[modeIndex][i]);
}
```
```json:testMetadata:mode-layer-matrix
{
  "testable": false,
  "skipReason": "Requires named child synths (Sustain, Release, PedalNoise, HammerNoise) in the module tree"
}
```

```javascript:bypass-missing-maps
// Title: Bypassing inactive articulation samplers
// Context: When loading a new patch, bypass any sampler that has no
// matching sample map and enable those that do.

const var articulationIds = ["sustain", "staccato", "legato"];
const var articulationSynths = [];

for (i = 0; i < articulationIds.length; i++)
    articulationSynths[i] = Synth.getChildSynth(articulationIds[i]);

inline function loadPatch(patchName)
{
    local availableMaps = Sampler.getSampleMapList();

    for (i = 0; i < articulationIds.length; i++)
    {
        local mapName = patchName + "_" + articulationIds[i];
        local hasMap = availableMaps.contains(mapName);

        articulationSynths[i].setBypassed(!hasMap);

        if (hasMap)
            articulationSynths[i].asSampler().loadSampleMap(mapName);
    }
}
```
```json:testMetadata:bypass-missing-maps
{
  "testable": false,
  "skipReason": "Requires named ModulatorSampler child synths and sample maps in the project"
}
```
