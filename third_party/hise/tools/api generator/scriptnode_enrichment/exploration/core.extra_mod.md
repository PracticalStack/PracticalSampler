# core.extra_mod - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/ModulationNodes.h:637`
**Base class:** `indexable_mod_base` -> `mod_base<NV, fix_hash<5000>, RuntimeTarget::ExternalModulatorChain, extra_config>`
**Classification:** audio_processor (modulation bridge)

## Signal Path

Picks up a modulation signal from an extra modulation chain of a hardcoded effect and exposes it inside scriptnode. The node uses `fix_hash<5000>` (CustomOffset) to connect specifically to extra modulation chains (not gain or pitch chains). The signal passes through in Raw mode with no transformation -- no intensity scaling, no mode formula, no Value parameter. The Index parameter selects which extra modulation chain slot to read from.

Signal flow: Hardcoded effect's extra mod chain -> ExtraModulatorRuntimeTargetSource -> runtime_target connection -> SignalSource -> EventData query per voice -> raw passthrough -> ModValue output (and optionally audio channel 0).

## Gap Answers

### extra-mod-source: What exactly is an 'extra modulation chain'?

Yes, extra modulation chains are the additional modulation slots on hardcoded effects -- for example, a filter's frequency modulation, or a reverb's room size modulation. These are modulation chains beyond the standard gain chain that a hardcoded effect can expose. The `ExtraModulatorRuntimeTargetSource` creates a SignalSource with one slot per extra mod chain, all sharing the fixed hash 5000 (CustomOffset). The Index parameter selects which slot to read.

### raw-mode-behaviour: Confirm raw passthrough with no transformation

Confirmed. The `extra_config` class returns `TargetMode::Raw` from `getMode()`. In `applyModulation()`, Raw mode hits the `return` statement immediately (line 191) -- no transformation is applied. The node has no Mode, Value, or Intensity parameters. The raw modulation signal (0-1 normalised from the HISE modulation chain) passes through unmodified.

### index-mapping: How do indices map to the parent effect's modulation chains?

Index 0 maps to the first extra modulation chain defined on the hardcoded effect, index 1 to the second, etc. The mapping is positional within the `SignalSource.modValueFunctions` array. Each slot corresponds to one `RuntimeTargetSource::getEventData` function for the respective chain. The range [0, 16] allows up to 17 extra chains, which is more than sufficient (most effects have 1-4 extra chains).

### description-missing: Correct short description

No SN_DESCRIPTION macro is present for extra_mod. A suitable description: "Picks up a modulation signal from an extra modulation chain of a hardcoded effect".

### display-buffer-usage: Is the display downsampled?

Yes. The display buffer uses `DisplayBufferDownsamplingFactor = 32` (inherited from mod_base). In `process()`, it writes `max(2, blockSize / 32)` samples per block. The buffer length is set to 2048 samples (65536 / 32). This is the standard mod_base display behavior.

## Parameters

- **Index** (0-16, step 1, default 0): Selects which extra modulation chain to read from within the parent hardcoded effect.
- **ProcessSignal** (Disabled/Enabled, default Disabled): When enabled, writes the raw modulation signal to audio channel 0.

## Polyphonic Behaviour

Same as global_mod -- per-voice state in `PolyData<Data, NV>` with per-voice EventData, uptime tracking, and smoothed baseValue/intensity (though the latter are not meaningfully used since Raw mode skips them). On note-on, queries the SignalSource for voice-specific EventData.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: [{"parameter": "ProcessSignal", "impact": "low", "note": "Enabled mode writes full buffer to channel 0; disabled mode evaluates single sample per block"}]

## Notes

- The `static_assert` enforces that IndexClass must be `ExtraIndexer` (`fix_hash<5000>`), preventing misuse at compile time.
- Since Raw mode bypasses applyModulation entirely, the smoothed baseValue and intensity members in the per-voice Data struct are allocated but not effectively used. This is a minor memory overhead, not a functional issue.
- extra_mod registers `NeedsModConfig` property, meaning it requires modulation configuration in the parent scope.
- The config class (`extra_config`) has virtual `prepare()` and `initialise()` methods for validation by the scriptnode wrapper.
