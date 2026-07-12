---
title: RNBO
description: "How to import RNBO patches as scriptnode DSP nodes"

guidance:
  summary: >
    Guide for importing DSP patches from Cycling '74 RNBO into HISE. Covers
    the export settings in RNBO, the HISE template builder for generating
    C++ wrapper code, the DLL compilation workflow, and advanced features
    including modulation output, tempo syncing, and complex data types
    (Tables, SliderPacks, AudioFiles). Assumes familiarity with scriptnode
    and the general concept of third-party C++ node integration.
  concepts:
    - RNBO
    - Max/MSP
    - third-party DSP
    - DLL compilation
    - scriptnode integration
    - modulation output
    - complex data types
    - template builder
  prerequisites:
    - scriptnode
    - cpp-dsp-nodes
  complexity: intermediate
---

[RNBO](https://rnbo.cycling74.com) is a subset of Max/MSP that exports DSP algorithms as portable C++ code. In HISE, RNBO patches are compiled into scriptnode nodes through a manual export→build→compile pipeline — you design your patch in RNBO, export C++ from it, generate a wrapper with the HISE template builder, compile a DLL, and use the resulting node. Unlike the $LANG.faust$ integration (which compiles automatically on save), each change to an RNBO patch requires a full re-export and recompile cycle.

The integration supports three areas beyond basic audio processing:

- **MIDI and tempo sync** — MIDI note on/off and CC messages pass through to the RNBO patch, and transport state (start/stop, BPM) is synced automatically.
- **Modulation output** — an `outport` object with the `mod` tag becomes a modulation source that can drive any parameter in the scriptnode network.
- **Complex data types** — HISE's Table, SliderPack, and AudioFile slots can be connected to `buffer~` objects in the RNBO patch for sharing float array data.

**See also:** [Usage in HISE](#usage-in-hise) -- export settings, wrapper generation, DLL compilation, and advanced features


## The Language

RNBO patches are designed in the Max/MSP visual programming environment — the language reference is Cycling '74's own [RNBO documentation](https://rnbo.cycling74.com/learn). This section covers only the RNBO-side objects and conventions that matter for the HISE integration.

### Parameters

RNBO UI elements (sliders, dials, number boxes) automatically become scriptnode parameters when the wrapper is compiled. Parameter names, ranges, and defaults are taken from the RNBO patch — no additional configuration is needed on the HISE side.

### Modulation via `outport`

An `outport` object with the tag ID `mod` becomes a modulation source in scriptnode. The value sent to this outport is forwarded as a normalised modulation signal to connected targets.

For buffer-synchronised modulation (matching scriptnode's `fix_block` resolution), two `inport` tags are available:

| Object | Tag | Purpose |
| --- | --- | --- |
| `inport` | `postrender` | Receives a bang after each audio buffer is processed — use this to trigger periodic modulation updates |
| `inport` | `blocksize` | Fires when processing specs change, provides the current block size |

### Data slots via `buffer~`

A `buffer~` object with the `@external 1` attribute becomes a data slot that HISE can connect to a Table, SliderPack, or AudioFile. The buffer's `@name` attribute is used to identify the slot in the HISE template builder.

```
buffer~ @name my_table @external 1
```

The name must be a valid C++ identifier (letters, digits, underscores; no spaces; doesn't start with a digit).


## Usage in HISE

The previous section covered the RNBO-side objects. This section covers the HISE-side workflow — exporting from RNBO, generating the wrapper, compiling the DLL, and configuring modulation and data slots.

### Export from RNBO

Open your patch in RNBO and navigate to the Export menu. The following settings must be configured for HISE compatibility:

| Setting | Value | Notes |
| --- | --- | --- |
| Output directory | `DspNetworks/ThirdParty/src` | Where HISE looks for exported RNBO code |
| ExportName | `your_patch_name` | Must be a valid C++ identifier and match ClassName exactly |
| ClassName | `your_patch_name` | Must match ExportName exactly |
| Polyphony | Disabled | RNBO polyphony must be off — use HISE's polyphony instead |

> [!Warning:ExportName and ClassName must be identical] If the two names don't match exactly, the template builder will generate wrapper code that doesn't compile. Both must be valid C++ identifiers: start with a letter or underscore, contain only letters, digits, and underscores, and not be a reserved keyword (`if`, `while`, `public`, etc.).

Click Export to generate the C++ files in the output directory.

### Build the wrapper and compile

Once the RNBO C++ files are in `DspNetworks/ThirdParty/src`, generate the HISE wrapper and compile:

1. In HISE, go to **Tools > Create Template for RNBO Patch**
2. Select the exported C++ file from the dropdown
3. Configure channels, polyphony, and other options — each has a help button explaining the setting
4. Press **OK** to generate the wrapper code in `ThirdParty/`
5. Go to **Export > Compile DSP Networks as DLL** to compile everything into a DLL
6. Restart HISE — the RNBO node now appears in scriptnode's node browser

The compiled node behaves like any other scriptnode node: it can be placed in networks, connected to parameters and modulation sources, and used inside polyphonic containers.

> [!Tip:Re-export after every RNBO change] Unlike Faust (which recompiles live), every change to your RNBO patch requires repeating the full export→template→DLL cycle. Keep the RNBO export settings saved so re-exporting is a single click.

**See also:** $LANG.cpp-dsp-nodes$ -- the C++ DSP node workflow (same DLL compilation pipeline)

### Route modulation output

To send a modulation signal from your RNBO patch to any scriptnode parameter:

1. Enable **Modulation** in the template builder when generating the wrapper
2. Add an `outport` object in your RNBO patch with the tag `mod`
3. For periodic modulation (once per audio buffer), add an `inport` with the tag `postrender` — it receives a bang after each buffer is processed
4. To know the buffer size, add an `inport` with the tag `blocksize` — it fires when processing specs change

The modulation resolution is controlled by `fix_block` containers in the scriptnode network, consistent with how modulation works everywhere else in scriptnode.

**Example: Peak Detector** (equivalent to `core.peak`) — the RNBO patch feeds a signal cable into `peakamp~`, uses the `postrender` inport for once-per-buffer updates, and sends the peak value through an `outport` with the `mod` tag.

### Connect complex data slots

HISE's three main UI data types — Table, SliderPack, and AudioFile — all operate on float arrays that can be shared with RNBO patches through `buffer~` objects.

To connect a data slot:

1. In RNBO, add a `buffer~` object with a unique `@name` attribute and `@external 1`
2. In the HISE template builder, enter the buffer name into the appropriate field (Table IDs, SliderPack IDs, or AudioFile IDs)
3. After compilation, the node exposes the corresponding data slot that you can connect to your interface

> [!Warning:AudioFile slots use double memory] RNBO expects interleaved audio data (all samples of one channel written consecutively). The wrapper duplicates the buffer and converts the format, which doubles memory usage for AudioFile slots. Multi-format audio files (SFZ, Samplemap) are not supported — only single audio files per slot.


## Differences from Standard RNBO

### Feature restrictions

Features from standard RNBO that are restricted or unavailable in the HISE integration.

| Feature | Standard RNBO | HISE | Rationale |
| --- | --- | --- | --- |
| Polyphony | Configurable in export settings | Must be disabled | HISE manages polyphony through its own voice system and polyphonic scriptnode containers |
| Audio file format | Interleaved or deinterleaved | Interleaved only — wrapper converts and duplicates the buffer | RNBO's interleaved format doesn't match HISE's internal layout |
| Multi-format audio | Supported | Not supported | One audio file per slot only — no SFZ or Samplemap |
| Export naming | Any string | Must be a valid C++ identifier, ExportName must equal ClassName | Required for the C++ wrapper to compile correctly |

### Integration differences

Features that exist in both standard RNBO and HISE but work differently due to the scriptnode hosting model.

| Feature | Standard RNBO | HISE | Rationale |
| --- | --- | --- | --- |
| Modulation output | Via `outport` (any tag) | Via `outport` with `mod` tag specifically | The wrapper looks for this tag to create a scriptnode modulation source |
| Compilation | Export once, use in any target | Export → template builder → DLL compile → restart | Each change requires the full pipeline; no live recompilation |
| UI elements | Rendered by target architecture | Become scriptnode parameters | Visual controls come from HISE's UI system |


**See also:** $LANG.faust$ -- alternative DSP language integration with live JIT compilation, $LANG.cpp-dsp-nodes$ -- writing custom C++ nodes directly
