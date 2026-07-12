# control.clone_cable - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1476`
**Base class:** `pimpl::no_processing`, `pimpl::parameter_node_base<ParameterType>`, `pimpl::templated_mode`, `wrap::clone_manager::Listener`
**Classification:** control_source

## Signal Path

Value parameter (or MIDI frequency for reactive modes) -> LogicType::getValue(index, numClones, value, gamma) per clone -> callEachClone(i, valueToSend) -> target parameter on each clone. The distribution function is determined by the LogicType template argument (duplilogic namespace), selectable at runtime via the Mode property.

## Gap Answers

### distribution-formulas: Exact mathematical formula for each duplilogic mode

All formulas from `logic_classes.h`, signature: `getValue(int index, int numUsed, double inputValue, double gamma)`.

**spread:** If numUsed==1, returns 0.5. Otherwise: `n = index/(numUsed-1) - 0.5`. If gamma != 0: `gn = sin(pi*n)*0.5; n = gn*gamma + n*(1-gamma)`. Then `n *= inputValue`. Returns `n + 0.5`. Output range centered at 0.5, spread width controlled by inputValue.

**scale:** If numUsed==1, returns inputValue. Otherwise: `n = index/(numUsed-1) * inputValue`. If gamma != 1.0: `n = pow(n, 1+gamma)`. Returns n. Linear ramp from 0 to inputValue.

**triangle:** If numUsed==1, returns 1.0. Otherwise: `n = abs(index/(numUsed-1) - 0.5) * 2.0`. If gamma != 0: `gn = sin(n*pi/2)^2; n = gamma*gn + (1-gamma)*n`. Returns `1.0 - inputValue*n`. V-shape peak at center, dips at edges.

**harmonics:** Returns `(index+1) * inputValue`. Integer harmonic multiples. Inherits from midi_logic::frequency.

**nyquist:** Calls harmonics().getValue() to get hvalue, then returns `smoothstep(hvalue, 1.0, min(0.99, gamma))`. Harmonic series with smoothstep rolloff. Inherits from midi_logic::frequency.

**fixed:** Returns inputValue unchanged for all clones. Inherits from midi_logic::frequency (so it responds to MIDI note-on frequency).

**ducker:** Returns `1.0 / max(1.0, numUsed)`. If gamma != 0: `pow(v, 1-gamma)`. Ignores index and inputValue entirely. Gain compensation based on clone count.

**random:** If numUsed==1, n=0.5, else `n = index/(numUsed-1)`. Returns `clamp(n + (2*random()-1)*inputValue, 0, 1)`. Spread-like base with random offset scaled by inputValue. Re-randomizes on each call to sendValue().

**toggle:** `thisIndex = index/numUsed`. Returns `1.0 if thisIndex <= inputValue, else 0.0`. Binary on/off threshold.

### gamma-effect-per-mode: How Gamma affects each mode

- **spread:** Blends between linear ramp and sine-curved ramp. At gamma=0, pure linear. At gamma=1, fully sine-curved (smoother center, sharper edges).
- **scale:** Power curve exponent. At gamma=0, exponent is 1 (pure power curve). At gamma=1, exponent is 2 (quadratic). Note: check is `gamma != 1.0` not `gamma != 0.0`.
- **triangle:** Blends between linear V-shape and sin^2 curve. At gamma=0, linear. At gamma=1, fully sin^2.
- **harmonics:** Gamma is ignored entirely (not used in getValue).
- **nyquist:** Controls smoothstep rolloff steepness. Higher gamma = steeper rolloff of higher harmonics. Clamped to max 0.99.
- **fixed:** Gamma is ignored (parameter not used).
- **ducker:** Power curve modifier. At gamma=0, pure 1/N. At gamma=1, `pow(1/N, 0)` = 1.0 (no ducking). Acts as a "less ducking" control.
- **random:** Gamma is ignored (not used in getValue).
- **toggle:** Gamma is ignored (not used in getValue).

### midi-reactive-modes-detail: How MIDI frequency updates the input value

Modes inheriting from `midi_logic::frequency<0>` (harmonics, nyquist, fixed) implement `getMidiValue(HiseEvent& e, double& v)`. On note-on: `v = e.getFrequency() / 20000.0`. This value REPLACES the Value parameter input -- in `clone_cable::handleHiseEvent()`, `v` starts as `lastValue`, but if `getMidiValue` returns true, `v` is overwritten with the frequency and `setValue(v)` is called, which updates `lastValue`. So note-on frequency replaces the knob value entirely.

The `random` mode also implements `getMidiValue` but only returns true on note-on without changing v -- it just triggers a re-randomization via sendValue().

When no MIDI context is present (no note-on events), the Value parameter knob value is used directly.

### toggle-mode-behaviour: Toggle mode threshold logic

Toggle uses: `thisIndex = (double)index / (double)numUsed`. Returns `(double)(thisIndex <= inputValue)` -- so 1.0 if the clone's fractional position is at or below the Value parameter, else 0.0. The Value parameter acts as a threshold: at Value=0.5, roughly the first half of clones get 1.0 and the rest get 0.0. Note: `shouldUpdateNumClones()` returns false for toggle, meaning clone count changes do NOT trigger a resend of values.

### numclones-auto-sync: NumClones auto-sync behaviour

In the constructor, `this->getParameter().setParentNumClonesListener(this)` registers clone_cable as a `clone_manager::Listener`. When the parent clone container changes its clone count, `numClonesChanged(int newSize)` is called, which calls `setNumClones(newNumClones)` (guarded by `shouldUpdateClones()` which delegates to `obj.shouldUpdateNumClones()`). The NumClones parameter can still be set manually via the knob, but in practice the auto-sync overwrites it whenever the container changes. For toggle mode (shouldUpdateNumClones=false), auto-sync is suppressed.

### normalised-output: Confirm normalised output

clone_cable does NOT inherit from `no_mod_normalisation`. The `no_processing` base class defines `isNormalisedModulation() = true`. Therefore output is normalised: values sent to clones are in [0,1] and target parameter ranges are applied. This is confirmed by the absence of any `UseUnnormalisedModulation` registration.

## Parameters

- **NumClones** (P=0): Integer 1-16. Number of active clones to address. Auto-synced from parent clone container.
- **Value** (P=1): 0.0-1.0. Input value fed to the distribution function. For MIDI-reactive modes, overwritten by note frequency on note-on.
- **Gamma** (P=2): 0.0-1.0. Shape modifier for the distribution curve. Effect varies by mode; ignored by harmonics, fixed, random, toggle.

## Conditional Behaviour

The Mode property selects the LogicType template argument at compile time via `duplilogic` namespace. At runtime (interpreted), `duplilogic::dynamic` wraps all modes behind a NodePropertyT<String> selector. Nine modes available: spread, scale, triangle, harmonics, nyquist, fixed, ducker, random, toggle.

MIDI event processing is conditional: `isProcessingHiseEvent()` is a constexpr that checks the LogicType. Only harmonics, nyquist, fixed, and random implement getMidiValue. The IsProcessingHiseEvent property is always registered in the constructor (for the dynamic case).

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: [{ "parameter": "NumClones", "impact": "linear", "note": "One getValue() call per active clone" }]

## Notes

The `sendValue()` method passes `!obj.shouldUpdateNumClones()` as the `ignoreCurrentNumClones` flag to `callEachClone`. For toggle mode this is true, meaning all total clones are addressed regardless of the current numClones value.
