# template.dry_wet - Composite Exploration

**Root container:** `container.split`
**Classification:** container (composite template)

## Signal Path

The root is a `container.split`, which duplicates the input signal to two parallel paths and sums their outputs.

Parallel paths:

**dry_path** (`container.chain`):
1. **dry_wet_mixer** (`control.xfader`) -- crossfade controller with Mode=Linear and NumParameters=2. Distributes complementary gain values to dry_gain and wet_gain based on the DryWet parameter. SwitchTarget[0] connects to dry_gain.Gain, SwitchTarget[1] connects to wet_gain.Gain.
2. **dry_gain** (`core.gain`) -- applies the dry portion of the crossfade to the dry signal path.

**wet_path** (`container.chain`):
1. **dummy** (`math.mul`, Value=1.0) -- placeholder node (multiply by 1.0 = passthrough). User replaces or adds wet DSP processing here.
2. **wet_gain** (`core.gain`) -- applies the wet portion of the crossfade to the wet signal path.

Parameter routing:
- Exposed **DryWet** (0..1) -> dry_wet_mixer.Value

The control.xfader in Linear mode with 2 targets produces complementary gain values: SwitchTarget[0] receives (1-x) and SwitchTarget[1] receives x, where x is the normalized Value parameter. This creates a linear crossfade: at DryWet=0, dry_gain=1/wet_gain=0 (fully dry); at DryWet=1, dry_gain=0/wet_gain=1 (fully wet); at DryWet=0.5, both gain values are 0.5.

The container.split sums the outputs of both paths. Since the xfader provides complementary gains that sum to 1.0 at all positions, the overall signal level is preserved (unity sum) at the extremes. At DryWet=0.5 with equal dry and wet content, the output is -6dB per path, summing to unity. This is a linear crossfade (not equal-power), so there is a slight level dip at the midpoint if dry and wet signals are uncorrelated.

## Gap Answers

### xfader-crossfade-curve

The control.xfader in Linear mode distributes values as a simple linear crossfade: target[0] = 1 - x, target[1] = x, where x is the normalized Value (DryWet). This is NOT an equal-power curve. At DryWet=0.5, both paths receive 0.5 gain (-6dB each). For uncorrelated signals, the sum at midpoint will be approximately -3dB below the level at the extremes. For correlated signals (identical dry and wet), the sum remains at unity throughout.

### split-summing-behaviour

The container.split sums the outputs of dry_path and wet_path. The xfader ensures complementary gains (dry + wet = 1.0), so at DryWet=0 or DryWet=1, one path is at unity and the other is silent -- the output is at unity gain. At intermediate values, both paths contribute proportionally. There is no additional gain compensation. The linear crossfade law means the sum of amplitudes is always 1.0, but the sum of powers dips at the midpoint for uncorrelated signals.

### description-accuracy

The base description "Processes each node independently and sums up the output" is inherited from container.split and does not describe the template's dry/wet mixing functionality. A more accurate description: "A dry/wet mix template with linear crossfade between unprocessed and processed signal paths."

### wet-path-user-workflow

The wet_path contains math.mul (dummy, Value=1.0 = passthrough) followed by wet_gain. The user should add their wet processing nodes before wet_gain, either by inserting nodes between the dummy and wet_gain, or by replacing the dummy entirely with their processing chain. The wet_gain node must remain at the end of the wet_path because it applies the crossfade gain. If the user removes wet_gain, the wet path will always be at full volume regardless of the DryWet setting.

## Parameters

- **DryWet** (0..1): Controls the crossfade balance. 0 = fully dry, 1 = fully wet. Routes to dry_wet_mixer.Value.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

The template itself adds only a split container, an xfader, and two gain nodes. Actual CPU cost depends on what the user places in the wet path.

## Notes

- The xfader node is placed inside dry_path (not at the split root level). This is because control nodes in scriptnode process within their container's signal flow but route their modulation output to any target. The xfader's switch targets cross the path boundary to reach wet_gain in wet_path.
- The dry_wet_mixer has Automated=true on its Value parameter, confirming it is intended to be driven by the exposed DryWet parameter.
- Available image: drywet.png
