# Accurate Salamander large-instrument qualification

Result: PASS

Signed by: DRS automated large-instrument qualification (Release)

Corpus: `E:/Dev/Cpp/VST/DecentRhapsody/DemoSFVInstruments/AccurateSalamanderGrandPianoV6.2beta2_48khz24bit/sfz_daw/Accurate-SalamanderGrandPiano_flat.Recommended.sfz`

- WAV files: 641
- Corpus WAV bytes: 1964042398
- Projected sources: 637
- Projected zones/routes: 1700
- Semantically analyzed regions: 1704
- Unsafe unconditional regions: 4
- Import analysis/projection: 250488 us
- Atomic authoring apply: 4084 us
- Full snapshot: 64397 us
- Selected-zone retained zones/sources: 6/6
- Selected-group retained zones/sources: 22/22
- Selected-zone preparation: 2800 us
- Selected-group preparation: 7927 us
- Full-draft preparation: 315021 us
- Estimated resident decoded bytes: 2613205608
- Full-draft decoded bytes: 0
- Full-draft resident-head bytes: 10436608
- Default middle-C eligible routes: 1
- Enabled middle-C auxiliary resonance routes: 2
- CC20 middle-C sampled-release routes: 2
- CC21 middle-C hammer routes: 1
- Retained random pedal-transition routes: 0
- Approved minimum-reference regions: 1408
- Standard/minimum reference peak: 0.235525/0.235525
- Standard/minimum reference RMS: 0.0387938/0.0387938
- Standard/minimum reference duration frames: 72064
- Standard/minimum maximum sample-aligned error: 0
- Standard/minimum RMS sample-aligned error: 0
- Standard/minimum maximum spectral-profile error: 0
- Selected-zone sustained peak/elapsed: 0.470763266/2515518 us
- Selected-group sustained peak/elapsed: 0.470763266/2500385 us
- Zone worker intents/prepared/failures: 3016/393/0
- Zone page cache bytes: 851968
- Zone maximum page read latency: 531 us
- Zone maximum callback duration/budget: 298/5333 us
- Zone intent published/consumed/dropped/max-depth: 3016/3016/0/16
- Zone cache budget/peak/leased/retired bytes: 402653184/851968/0/0
- Zone page misses/underrun frames/recoveries: 0/0/0
- Group intents/prepared/failures: 3016/401/0
- Group page misses/underrun frames/recoveries: 0/0/0
- Constrained profile: 75 ms page-service poll
- Constrained peak/elapsed: 0.470763266/2496801 us
- Constrained intents/prepared/failures: 264/33/0
- Constrained page misses/underrun frames/recoveries: 695328/695328/8
- Peak process working set: 201990144 bytes

## Package v2

- Package bytes: 2626855559
- Records: 40782
- Export elapsed: 15116326 us
- Export throughput: 173279642 plaintext bytes/s
- Peak plaintext/sealed buffers: 65536/65592 bytes
- Structural verification bytes: 5256530
- Metadata open: 224928 us
- Warm metadata reopen: 20617 us
- Head-ready/playable: 71799 us
- Package resident-head bytes: 10436608
- Package first-note peak: 0.0588454157
- Actual-corpus cancellation latency/polls: 37307110 us/22048
- Actual-corpus cancellation bytes processed: 67189143

The run used real corpus WAV descriptors, bounded 16 KiB heads, the production page-intent worker, the immutable render model, and callback-side activation.
