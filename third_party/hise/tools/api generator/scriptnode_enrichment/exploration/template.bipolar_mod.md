# template.bipolar_mod - Composite Exploration

**Root container:** `container.modchain`
**Classification:** container (composite template)

## Signal Path

The root is a `container.modchain`, which wraps all children in `wrap::fix<1, wrap::control_rate<...>>`. This means the entire chain runs at mono, control rate (decimated by HISE_EVENT_RASTER, typically factor 8 for instrument plugins, factor 1 for effect plugins).

Internal serial chain (all at control rate, mono):

1. **mod_signal** (`container.chain`) -- user-replaceable modulation source. Default contains `core.ramp` as a dummy placeholder (1 second period, gate=1).
2. **sig2mod** (`math.sig2mod`) -- converts audio-range signal (-1..1) to modulation range (0..1) via the formula `output = (input + 1.0) * 0.5`.
3. **peak** (`core.peak`) -- captures the current signal amplitude and outputs it as a modulation value. Modulation target: bipolar.Value.
4. **bipolar** (`control.bipolar`) -- transforms the 0..1 input into a bipolar range (-Scale..+Scale) with optional gamma curve. Modulation target: pma.Add.
5. **pma** (`control.pma`) -- combines base Value with bipolar offset: `output = Value * Multiply + Add`. The user connects pma's modulation output to their target parameter.

Parameter routing:
- Exposed **Value** (0..1) -> pma.Value (the base/centre value)
- Exposed **Intensity** (0..1) -> bipolar.Scale (modulation depth, maps to -Scale..+Scale range)

Internal modulation connections:
- core.peak modulates -> control.bipolar.Value
- control.bipolar modulates -> control.pma.Add

## Gap Answers

### internal-signal-flow-verification

The root container.modchain wraps children in `wrap::fix<1, wrap::control_rate<...>>`. This applies to the entire child chain -- all nodes (mod_signal, sig2mod, peak, bipolar, pma) run at control rate in mono. The timing resolution is sampleRate / HISE_EVENT_RASTER. For instrument plugins at 44100 Hz with HISE_EVENT_RASTER=8, this gives ~5512 Hz control rate. For effect plugins where HISE_EVENT_RASTER=1, it runs at full sample rate but still mono.

### pma-output-connection

The pma node (control.pma) is a control node that produces modulation output. In the base JSON, its ModulationTargets is empty, meaning no target is connected by default. The user must manually drag a modulation connection from the pma node's modulation output to their desired target parameter. This is the standard scriptnode workflow for connecting modulation sources to targets. The pma node computes `Value * Multiply + Add` and sends the result as its modulation output.

### description-accuracy

The base description "a serial chain optimized for modulation sources" is inherited from container.modchain and does not describe this template's specific purpose. A more accurate description would be: "A bipolar modulation source template that converts a 0..1 signal into a bipolar offset around a base value, with adjustable intensity."

### user-replaceable-source

The mod_signal chain contains a core.ramp as a dummy placeholder (note the comment in the JSON: "Create a signal between 0...1 here"). The intended workflow is for the user to delete the ramp and insert their own modulation source node(s) that produce a signal in the -1..1 audio range (sig2mod will convert to 0..1). Users could also add nodes alongside the ramp within the mod_signal chain.

## Parameters

- **Value** (0..1): The base/centre value for the modulation output. Routes to pma.Value. When Intensity is 0, the pma output equals this value.
- **Intensity** (0..1): Modulation depth. Routes to bipolar.Scale. At 0, no modulation offset is applied. At 1, the full bipolar range (-1..+1) is added to the base value.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

The entire template runs at control rate, and all child nodes are lightweight control/math operations.

## Notes

- The pma node has a Comment property: "Connect this to the target knob" -- confirming the user workflow.
- The core.ramp dummy has PeriodTime=1000ms, which creates a slow 1Hz sawtooth for testing purposes.
- The pma.Multiply parameter defaults to 1.0 and is not exposed, so the formula simplifies to `output = Value + Add` where Add comes from the bipolar modulation.
