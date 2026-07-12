# ModuleIds -- Class Analysis

## Brief
Constants-only namespace listing all available module type IDs for the owner synth's chains.

## Purpose
ModuleIds is a dynamic constants namespace that provides type-safe string identifiers for every processor type available in the owner synth's internal chains (MIDI processors, modulators, and effects). The constants are collected at construction time by iterating all internal chains of the owner `ModulatorSynth`, querying each chain's `FactoryType` for its allowed processor types, deduplicating, and sorting alphabetically. Each constant maps a name to itself as a string value (e.g., `ModuleIds.LFO` evaluates to `"LFO"`). The primary consumer is `Synth.addModule()` which uses these type ID strings to instantiate new processors.

## Details

### Constant Population Mechanism

The constructor calls `getTypeList(ownerSynth)` which:
1. Iterates `s->getNumInternalChains()` (typically 4 for a standard `ModulatorSynth`: MidiProcessor, GainModulation, PitchModulation, EffectChain)
2. For each chain, obtains its `FactoryType` via `c->getFactoryType()`
3. Calls `getAllowedTypes()` which returns `ProcessorEntry` objects filtered by any active constrainers
4. Collects unique `ProcessorEntry.type` identifiers across all chains

The resulting list is sorted alphabetically and each entry is registered as a constant where both name and value are the type ID string.

### Chain-to-Module Categories

The available constants span four module categories based on the standard `ModulatorSynth` chains:

| Chain | Factory Type | Module Categories |
|-------|-------------|-------------------|
| MidiProcessor (0) | MidiProcessorFactoryType | Script processors, transposer, MIDI player, hardcoded MIDI scripts |
| GainModulation (1) | ModulatorChainFactoryType | Voice-start modulators, time-variant modulators, envelope modulators |
| PitchModulation (2) | ModulatorChainFactoryType | Same modulator types as GainModulation (shared factory) |
| EffectChain (3) | EffectProcessorChainFactoryType | Filters, reverbs, delays, dynamics, scriptnode FX, etc. |

Because GainModulation and PitchModulation share the same factory types, modulators appear only once in ModuleIds (deduplication via `addIfNotAlreadyThere`).

### Type ID vs Display Name

The type ID (what ModuleIds provides) is often different from the human-readable display name. Examples:

| ModuleIds Constant | Display Name | Category |
|-------------------|-------------|----------|
| `LFO` | LFO Modulator | Time Variant Modulator |
| `AHDSR` | AHDSR Envelope | Envelope Modulator |
| `Velocity` | Velocity Modulator | Voice Start Modulator |
| `PolyphonicFilter` | Filter | Effect |
| `SimpleGain` | Simple Gain | Effect |
| `StreamingSampler` | Sampler | Sound Generator |
| `ScriptProcessor` | Script Processor | MIDI Processor |

### Synth Types Not Included

Sound generator types (SineSynth, WaveSynth, StreamingSampler, etc.) are NOT available via ModuleIds because they belong to `ModulatorSynthChainFactoryType`, not to any internal chain. Use the `Builder` class constants for synth type IDs instead.

### Owner Synth Dependency

The constant set depends on the specific `ModulatorSynth` that owns the script processor. Synth subclasses with additional internal chains (e.g., `WavetableSynth`, `ModulatorSampler`) may expose additional processor types.

## obtainedVia
Global namespace -- available as `ModuleIds` in any script processor.

## minimalObjectToken


## Constants
Dynamic constants only -- see Dynamic Constants section.

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| (all processor type IDs) | String | Each constant is a processor type identifier string. The exact set depends on the owner synth's chains and their factory types. Typical count is ~50+ constants covering MIDI processors, modulators, and effects. |

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Synth.addModule(0, ModuleIds.SineSynth, "s")` | Use `Builder` for synth types | ModuleIds does not contain sound generator type IDs -- only chain-level processor types (MIDI, modulators, effects). |
| `Synth.addModule(3, "Filter", "f")` | `Synth.addModule(3, ModuleIds.PolyphonicFilter, "f")` | The type ID is the internal C++ identifier, not the display name. Use ModuleIds constants to avoid typos. |

## codeExample
```javascript
// ModuleIds provides type-safe constants for Synth.addModule()
// Add an LFO modulator to the gain modulation chain
Synth.addModule(Synth.Chain.Gain, ModuleIds.LFO, "GainLFO");

// Add a filter effect to the FX chain
Synth.addModule(Synth.Chain.FX, ModuleIds.PolyphonicFilter, "MainFilter");
```

## Alternatives
Builder -- provides its own module type constants (including synth types) for programmatic module tree construction.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Constants-only namespace with no methods -- no call-site validation possible. The string values are consumed by Synth.addModule() which has its own validation.
