## clear

**Examples:**

```javascript:clear-rebuild-multi-output
// Title: Clear-and-rebuild pattern for toggling multi-output mode
// Context: When switching between stereo and multi-output modes, clear
// the matrix first, then rebuild the desired routing from scratch.

const var masterMatrix = Synth.getRoutingMatrix("MasterSynth");
masterMatrix.setNumChannels(16);

inline function onMultiOutToggle(component, value)
{
    masterMatrix.clear();

    if (value)
    {
        // Multi-out: pass all stereo pairs through
        for (i = 0; i < 16; i++)
        {
            masterMatrix.addConnection(i * 2, i * 2);
            masterMatrix.addConnection(i * 2 + 1, i * 2 + 1);
        }
    }
    else
    {
        // Stereo: fold to main output
        masterMatrix.addConnection(0, 0);
        masterMatrix.addConnection(1, 1);
    }
}

const var multiOutButton = Content.getComponent("MultiOutButton");
multiOutButton.setControlCallback(onMultiOutToggle);
```
```json:testMetadata:clear-rebuild-multi-output
{
  "testable": false,
  "skipReason": "Requires named processor 'MasterSynth' with multichannel support and UI component 'MultiOutButton'"
}
```

```javascript:isolate-builder-effect-channels
// Title: Isolating a Builder-created effect to specific channels
// Context: When creating effects in a multichannel signal path,
// clear the default stereo routing first, then connect only the
// target channel pair.

const var b = Synth.createBuilder();

local fx = b.create(b.Effects.HardcodedMasterFX, "ReverbA", parentProc, b.ChainIndexes.FX);
local fxMatrix = b.get(fx, b.InterfaceTypes.RoutingMatrix);

// Remove the default stereo passthrough (0->0, 1->1)
fxMatrix.clear();

// Route only channels 2/3 through this effect
fxMatrix.addConnection(2, 2);
fxMatrix.addConnection(3, 3);

b.flush();
```
```json:testMetadata:isolate-builder-effect-channels
{
  "testable": false,
  "skipReason": "References undefined 'parentProc' builder variable from surrounding project context"
}
```

The `clear()` then `addConnection()` pattern is the standard way to reconfigure routing. Calling `addConnection` without clearing first leaves previous connections in place, which can cause unexpected signal summing or crosstalk between channel pairs.
