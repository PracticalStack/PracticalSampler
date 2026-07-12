---
title: Faust
description: "Integrating the Faust DSP language into HISE via the core.faust scriptnode node"

guidance:
  summary: >
    Guide for using the Faust functional DSP language inside HISE. Covers the
    core.faust scriptnode node, automatic compilation during development,
    parameter handling (automatic slider/button mapping to scriptnode
    parameters, MIDI zone auto-mapping for freq/gate/gain), channel count
    resolution (a common pain point - Faust output count must equal HISE
    channel count), modulation output via bargraph objects, external data
    limitations, the static export workflow for compiled plugins, and
    comprehensive differences from standard Faust. Assumes familiarity
    with scriptnode basics.
  concepts:
    - Faust
    - core.faust
    - compilation
    - parameter mapping
    - MIDI zone
    - freq gate gain
    - channel count
    - modulation output
    - bargraph
    - static export
    - DLL compilation
  prerequisites:
    - scriptnode
  complexity: intermediate
---

[Faust](https://faust.grame.fr) is a functional programming language for real-time audio signal processing. In HISE, Faust is a guest language inside scriptnode — your `.dsp` code compiles automatically during development and is transpiled to C++ for the final plugin binary. You get the full Faust standard library but operate within scriptnode's channel model and parameter system.

The main reasons to choose Faust inside HISE:

- **Instant compilation** — edit your `.dsp` file in the HISE code editor and changes compile automatically. No manual export step during development.
- **Automatic parameter mapping** — Faust UI elements (`hslider`, `button`, `nentry`) become scriptnode parameters with no glue code. Parameters named `freq`, `gate`, `gain` auto-map to MIDI.
- **Modulation output** — `hbargraph` and `vbargraph` objects become modulation sources that can drive any parameter in the scriptnode network.

The tradeoffs vs other DSP languages in HISE:

- No access to external data slots (Tables, SliderPacks, AudioFiles) — use $LANG.cpp-dsp-nodes$ if you need those
- Channel count must exactly match the scriptnode context
- Faust's own polyphony system is ignored — use HISE's voice management instead

> [!Tip:Compare approaches across DSP languages] The same DSP can be implemented in Faust, $LANG.snex$, and $LANG.cpp-dsp-nodes$. SNEX gives you the scriptnode callback API directly; C++ DSP nodes give you full JUCE access; Faust gives you the most concise signal-flow syntax. See the Saturated Sine example in the $LANG.snex$ reference for a side-by-side comparison.

**See also:** [Usage in HISE](#usage-in-hise) -- connecting Faust to scriptnode, parameter mapping, export workflow


## The Language

Faust is a well-documented language with its own [comprehensive documentation](https://faustdoc.grame.fr). This section covers only what you need to know to write Faust specifically for HISE — the `process` expression, how UI elements map to parameters, and how bargraph objects become modulation sources. For full language syntax, refer to the Faust documentation.

### The `process` expression

Every Faust program defines a `process` expression that describes the DSP algorithm as a signal flow graph. The number of inputs and outputs is determined by the expression's signal signature:

```faust
import("stdfaust.lib");

// Stereo passthrough (2 in, 2 out)
process = _, _;

// Mono-to-stereo (1 in, 2 out)
process = _ <: _, _;

// Stereo effect (2 in, 2 out)
process = _, _ : + : fi.lowpass(2, 1000) <: _, _;
```

The output count of `process` must match the channel count of your scriptnode network — see [Resolve channel count](#resolve-channel-count) below. This is the most common source of errors when starting with Faust in HISE.

### UI elements as parameters

Faust UI elements declare parameters with a name, default value, range, and step size. Each element becomes one scriptnode parameter when the node compiles:

```faust
import("stdfaust.lib");
gain = hslider("Gain", 0, -96, 0, 0.1);
freq = hslider("Frequency", 1000, 20, 20000, 1);
process = _ * ba.db2linear(gain) : fi.lowpass(2, freq) <: _, _;
```

This creates a node with two parameters — **Gain** (range -96 to 0, step 0.1) and **Frequency** (range 20 to 20000, step 1). The parameter names, ranges, and defaults are all taken directly from the Faust declaration.

| UI Element | Typical use |
| --- | --- |
| `hslider(name, default, min, max, step)` | Continuous parameter (knob/slider) |
| `vslider(name, default, min, max, step)` | Same as hslider (orientation ignored in HISE) |
| `nentry(name, default, min, max, step)` | Numeric entry (behaves like a slider in HISE) |
| `button(name)` | Momentary trigger (0 or 1) |
| `checkbox(name)` | Toggle (0 or 1) |

> [!Warning:Faust UI rendering is not used] Faust's own UI widget rendering is completely bypassed in HISE. The widgets exist only to declare parameters — visual controls come from scriptnode's parameter system or your HiseScript UI.

### Bargraph objects as modulation

`hbargraph` and `vbargraph` objects become modulation sources instead of visual displays. The bargraph value is sent as a normalised signal (0.0 to 1.0) to connected modulation targets in the scriptnode network:

```faust
import("stdfaust.lib");
process = _ : *(0.5) : attach(_, abs : hbargraph("mod", 0, 1));
```

Up to 4 modulation outputs are supported by default. Set `HISE_NUM_MAX_FAUST_MOD_SOURCES` in your project's preprocessor definitions to increase this (maximum 16).

### Standard libraries

One of Faust's strongest features is its comprehensive [library of DSP algorithms](https://faustlibraries.grame.fr). The full standard library collection is available in HISE via `import("stdfaust.lib")`. Functions you'll use most often:

| Library | Prefix | Common functions |
| --- | --- | --- |
| `basics.lib` | `ba.` | `ba.db2linear`, `ba.linear2db`, `ba.sAndH` |
| `filters.lib` | `fi.` | `fi.lowpass`, `fi.highpass`, `fi.resonbp`, `fi.svf` |
| `oscillators.lib` | `os.` | `os.osc`, `os.sawtooth`, `os.square`, `os.phasor` |
| `maths.lib` | `ma.` | `ma.SR` (sample rate), `ma.PI`, `ma.T60` |
| `signals.lib` | `si.` | `si.smoo` (smoothing), `si.bus`, `si.block` |
| `delays.lib` | `de.` | `de.delay`, `de.fdelay`, `de.sdelay` |
| `envelopes.lib` | `en.` | `en.adsr`, `en.asr`, `en.ar` |
| `effects.lib` | `ef.` | `ef.echo`, `ef.transpose` |
| `reverbs.lib` | `re.` | `re.mono_freeverb`, `re.stereo_freeverb`, `re.zita_rev1` |
| `physmodels.lib` | `pm.` | `pm.chain`, `pm.clarinetModel`, `pm.fluteModel` |

> [!Warning:Check per-function licenses before shipping] Faust libraries use **per-function licensing** — most functions are MIT-style (STK-4.3) or LGPL with a compiler-output exception, but some specific algorithms are **GPL-3.0** (e.g., `vital_rev` and `kb_rom_rev1` in `reverbs.lib`). GPL-licensed functions cannot be used in proprietary plugins. Check the `declare ... license` statement in the library source for each function you use. The full library reference at [faustlibraries.grame.fr](https://faustlibraries.grame.fr) documents each function individually.


## Usage in HISE

The previous section covered Faust as a language. This section covers how Faust connects to HISE — wiring up the scriptnode node, parameter and MIDI mapping, modulation routing, channel count rules, and the export workflow for production builds.

### Build HISE with Faust support

Faust is not enabled in HISE by default — you need to build HISE from source with the Faust build configuration selected. The setup differs by platform.

**Windows:**

1. Download and install Faust from [github.com/grame-cncm/faust/releases](https://github.com/grame-cncm/faust/releases) to the default location (`C:\Program Files\Faust`)
2. Open `projects/standalone/HISE Standalone.jucer` and select **Save and Open in IDE**
3. In Visual Studio, change the build configuration dropdown to **Debug with Faust** or **Release with Faust**
4. Build — HISE will display a text label in the top bar confirming Faust is enabled

**macOS:**

1. Download the Faust `.dmg` for your architecture from [github.com/grame-cncm/faust/releases](https://github.com/grame-cncm/faust/releases) (Intel: `Faust-VERSION-x64.dmg`, Apple Silicon: `Faust-VERSION-arm64.dmg`)
2. Extract the `include`, `bin`, `lib`, and `share` folders into `HISE_ROOT/tools/faust/` (alongside the existing `fakelib` folder)
3. In Projucer, go to **Exporters > Xcode (macOS) > Valid Architectures** and deselect any architectures that don't match your machine (remove `x86_64` on Apple Silicon, or `arm64`/`arm64e` on Intel) to avoid linker errors
4. In Xcode, set the build configuration to **Debug with Faust** or **Release with Faust** via **Product > Scheme > Edit Scheme > Build Configuration**
5. After building, open HISE Settings and set `FaustPath` to `HISE_ROOT/tools/faust/`

**Linux:**

Build instructions are available at [resonant-bytes.de/blog/gsoc-final-submission](https://resonant-bytes.de/blog/gsoc-final-submission/). After building, set the Faust installation directory in the HISE settings under `FaustPath`.

> [!Warning:Version compatibility] HISE builds against Faust 2.54.0+ by default. If you need to use the older Faust 2.50.6, enable the `HI_FAUST_NO_WARNING_MESSAGES` flag in the `hi_faust_types` Projucer module and recompile.

### Connect Faust to scriptnode

Faust code runs inside the $SN.core.faust$ scriptnode node. Add one to your network and the node's header bar provides:

- **Class selector** (ComboBox) — choose an existing `.dsp` file or create a new one
- **Add/Edit/Reload buttons** — manage source files and trigger recompilation
- **Modulation output dragger** — visible when the code declares bargraph objects

Faust source files live in `DspNetworks/CodeLibrary/faust/`. When you create a new class, HISE generates a default stereo passthrough template. Edit the file in the built-in code editor (which provides Faust syntax highlighting and error reporting) or in an external editor by enabling the `FaustExternalEditor` setting.

Compilation is automatic: save the file and the node recompiles and starts processing audio immediately. Parameter values are preserved across recompilation cycles, so you don't lose your current settings while iterating on the DSP code.

> [!Tip:Use the built-in editor for error feedback] The HISE code editor shows Faust compilation errors inline with line numbers. External editors work fine for writing code, but you'll need to check the HISE console for error messages.

**See also:** $SN.core.faust$ -- full node reference

### Map parameters and MIDI

Every Faust UI element (`hslider`, `button`, `nentry`, etc.) automatically becomes a scriptnode parameter when the node compiles. The parameter name, range, default, and step size are all taken from the Faust declaration — no glue code or manual registration needed.

Three parameter names are reserved for automatic MIDI mapping:

| Parameter name | MIDI mapping |
| --- | --- |
| `freq` | Note frequency in Hz (from MIDI note number) |
| `gate` | 1.0 on note-on, 0.0 on note-off |
| `gain` | Velocity-scaled gain (0.0 to 1.0) |

When your Faust code declares parameters with these exact names (lowercase), HISE maps them to MIDI note-on/note-off events automatically. This enables polyphonic Faust instruments without explicit MIDI handling:

```faust
import("stdfaust.lib");
freq = hslider("freq", 440, 20, 20000, 1);
gate = button("gate");
gain = hslider("gain", 1, 0, 1, 0.01);
process = os.osc(freq) * gate * gain <: _, _;
```

> [!Warning:Use exact names for MIDI mapping] The names must be exactly `freq`, `gate`, and `gain` (lowercase). Other names like `frequency` or `volume` will not auto-map. Conversely, avoid these names for parameters that shouldn't respond to MIDI — they will be mapped regardless.

### Route modulation output

Bargraph objects in your Faust code become modulation sources that can drive any parameter in the scriptnode network. Connect them by dragging from the modulation output on the `core.faust` node header to the target parameter.

For once-per-buffer modulation updates (matching scriptnode's `fix_block` resolution), use `inport` objects:

- `inport` with tag `postrender` — receives a bang after each audio buffer is processed
- `inport` with tag `blocksize` — called when processing specs change, provides the current block size

This keeps Faust modulation output consistent with how modulation works in the rest of scriptnode.

### Resolve channel count

The channel count rules are strict and are the most common source of errors for new users:

1. **Faust output count must equal the scriptnode network's channel count.** If your network is stereo (2 channels), `process` must output exactly 2 channels.
2. **Faust input count must be less than or equal to the channel count.** A mono-to-stereo effect (1 in, 2 out) is valid in a stereo context, but more inputs than the network provides is an error.

A mismatch produces a channel count error showing the expected and actual counts. The fix is usually adding a split operator to match the output count:

```faust
// ERROR in stereo context: only 1 output
process = _ : fi.lowpass(2, 1000);

// FIX: split mono output to stereo
process = _ : fi.lowpass(2, 1000) <: _, _;
```

> [!Warning:Check channel count first when debugging errors] If your Faust node shows a compilation error, check the channel count before looking at the DSP logic. The error message shows the expected and actual counts — it's almost always a missing `<: _, _` at the end of the signal chain.

### Export for production

During development, Faust code compiles automatically via libfaust. For the final plugin build, Faust code is transpiled to C++ and compiled into the binary:

1. Go to **Export > Compile DSP Networks as DLL**
2. HISE transpiles each `.dsp` file to C++ and generates the necessary wrapper code
3. The generated C++ is compiled alongside any other third-party nodes into a DLL

The live Faust compilation used during development is not available in exported plugins — the static C++ export is the only path to production.

This requires a working Faust installation as described in [Build HISE with Faust support](#build-hise-with-faust-support) — the same `FaustPath` setting is used for both live compilation and static export.

> [!Warning:Always re-export after Faust changes] If you modify your `.dsp` files after the last DLL export, you must re-export before building the plugin. Otherwise the compiled DLL contains stale code that doesn't match what you tested during development.

**See also:** $LANG.cpp-dsp-nodes$ -- the C++ DSP node workflow (same DLL export pipeline), $LANG.rnbo$ -- alternative external DSP integration


## Differences from Standard Faust

HISE's Faust integration provides full access to the language and standard libraries, but operates within scriptnode's runtime model. The differences fall into three categories.

### Feature omissions

Features from standard Faust that are not available inside HISE's integration. These are limitations of the host wrapper, not the language itself.

| Feature | Standard Faust | HISE |
| --- | --- | --- |
| `soundfile` primitive | Load audio files at runtime | Not supported |
| External data (tables, audio files) | Via `soundfile` or `waveform` | Not supported — use Faust's built-in `rdtable`/`rwtable` for compile-time tables, or use a $LANG.cpp-dsp-nodes$ node for runtime table access |
| Polyphony declarations | `declare options "[nvoices:N]"` | Ignored — use HISE's voice management and polyphony system instead |

### Integration differences

Features that exist in both standard Faust and HISE but behave differently due to the scriptnode hosting model.

| Feature | Standard Faust | HISE | Rationale |
| --- | --- | --- | --- |
| Channel count | Flexible — determined by architecture file | Must match scriptnode network (outputs = channels, inputs ≤ channels) | Scriptnode processes fixed-width channel buffers |
| UI widgets | Rendered by Faust architecture | Become scriptnode parameters — no Faust UI rendering | Visual controls come from HISE's UI system |
| Processing model | In-place possible | Input buffer always copied | Wrapper copies input to avoid conflicts with scriptnode's buffer model |
| Compilation | `faust` CLI or libfaust | Automatic in IDE during development; static C++ at export | Transparent to the user — no manual compilation step |
| Bargraph objects | Display-only value readouts | Become modulation sources driving scriptnode parameters | Repurposed to fit scriptnode's modulation routing |

### HISE-specific additions

Features the HISE integration adds on top of standard Faust.

| Feature | Description |
| --- | --- |
| MIDI auto-mapping | Parameters named `freq`, `gate`, `gain` are automatically mapped to MIDI note events — same convention as standard Faust, but handled automatically without architecture file setup |
| Modulation output | Bargraph objects become normalised modulation sources routable to any scriptnode parameter |
| `inport` modulation tags | `postrender` and `blocksize` inport tags for buffer-synchronised modulation updates |
| Automatic recompilation | Save-and-compile workflow with parameter persistence across recompilation cycles |


**See also:** $SN.core.faust$ -- the scriptnode node reference, $LANG.snex$ -- HISE's built-in JIT-compiled DSP language, $LANG.cpp-dsp-nodes$ -- full C++ DSP node development, $LANG.rnbo$ -- alternative external DSP integration
