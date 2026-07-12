Returns an object containing named constants for all available filter modes. Each constant maps a filter type name (like `StateVariableLP` or `MoogLP`) to its integer index, which you can pass to a filter effect's Mode attribute. Use this to build filter type selectors without hardcoding numeric indices.

| Constant | Description |
|----------|-------------|
| `LowPass` | Standard low-pass filter |
| `HighPass` | Standard high-pass filter |
| `LowShelf` | Low shelf EQ |
| `HighShelf` | High shelf EQ |
| `Peak` | Parametric peak/bell EQ |
| `ResoLow` | Resonant low-pass |
| `StateVariableLP` | State variable low-pass |
| `StateVariableHP` | State variable high-pass |
| `MoogLP` | Moog-style ladder low-pass |
| `OnePoleLowPass` | One-pole low-pass (6dB/oct) |
| `OnePoleHighPass` | One-pole high-pass (6dB/oct) |
| `StateVariablePeak` | State variable peak |
| `StateVariableNotch` | State variable notch |
| `StateVariableBandPass` | State variable band-pass |
| `Allpass` | All-pass filter |
| `LadderFourPoleLP` | 4-pole ladder low-pass |
| `LadderFourPoleHP` | 4-pole ladder high-pass |
| `RingMod` | Ring modulation |

Store the filter mode constants in an array indexed by combo box values to map user selections to the correct filter mode integer. This works with any module that accepts a filter mode parameter, including PolyphonicFilter and the FilterDisplay floating tile.