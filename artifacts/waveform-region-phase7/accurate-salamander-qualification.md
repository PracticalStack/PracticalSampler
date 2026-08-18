# Accurate Salamander large-instrument qualification

Result: PASS

Signed by: DRS automated large-instrument qualification (Release)

Corpus: `E:/Dev/Cpp/VST/DecentRhapsody/DemoSFVInstruments/AccurateSalamanderGrandPianoV6.2beta2_48khz24bit/sfz_live/Accurate-SalamanderGrandPiano_flat.Recommended.sfz`

- WAV files: 641
- Corpus WAV bytes: 1964042398
- Projected sources: 637
- Projected zones/routes: 1700
- Semantically analyzed regions: 1704
- Unsafe unconditional regions: 4
- Import analysis/projection: 325064 us
- Atomic authoring apply: 5788 us
- Full snapshot: 69824 us
- Selected-zone retained zones/sources: 6/6
- Selected-group retained zones/sources: 22/22
- Selected-zone preparation: 76511 us
- Selected-group preparation: 150261 us
- Full-draft preparation: 5863113 us
- Estimated resident decoded bytes: 2613205608
- Full-draft decoded bytes: 0
- Full-draft resident-head bytes: 10436608
- Full-draft region prewarm intents accepted/rejected: 0/0
- Default middle-C eligible routes: 1
- Enabled middle-C auxiliary resonance routes: 2
- CC20 middle-C sampled-release routes: 2
- CC21 middle-C hammer routes: 1
- Retained random pedal-transition routes: 0
- Approved minimum-reference regions: 1408
- Standard/minimum reference peak: 0.235427/0.235525
- Standard/minimum reference RMS: 0.0379853/0.037864
- Standard/minimum reference duration frames: 71912
- Standard/minimum maximum sample-aligned error: 0.212897
- Standard/minimum RMS sample-aligned error: 0.0324995
- Standard/minimum maximum spectral-profile error: 0.000415425
- Standard/minimum strict parity enforced: no (live-preset qualification)
- Standard/minimum within strict tolerance: no
- Selected-zone sustained peak/elapsed: 0.506415427/2614961 us
- Selected-group sustained peak/elapsed: 0.506415427/2611084 us
- Zone worker intents/prepared/failures: 3016/393/0
- Zone page cache bytes: 851968
- Zone maximum page read latency: 2211 us
- Zone maximum callback duration/budget: 406/5333 us
- Zone intent published/consumed/dropped/max-depth: 3016/3016/0/16
- Zone cache budget/peak/leased/retired bytes: 402653184/851968/0/0
- Zone page misses/underrun frames/recoveries: 0/0/0
- Group intents/prepared/failures: 3016/385/0
- Group page misses/underrun frames/recoveries: 0/0/0
- Constrained profile: 75 ms page-service poll
- Constrained peak/elapsed: 0.506415427/2607990 us
- Constrained intents/prepared/failures: 272/34/0
- Constrained page misses/underrun frames/recoveries: 704768/704768/8
- Peak process working set: 255008768 bytes

## Package v2

- Package bytes: 2631113592
- Records: 40846
- Export elapsed: 22015869 us
- Export throughput: 119168487 plaintext bytes/s
- Peak plaintext/sealed buffers: 65536/65592 bytes
- Structural verification bytes: 5264717
- Metadata open: 283507 us
- Warm metadata reopen: 16582 us
- Head-ready/playable: 72956 us
- Package resident-head bytes: 10436608
- Package first-note peak: 0.0633019358
- Actual-corpus cancellation latency/polls: 37086291 us/22047
- Actual-corpus cancellation bytes processed: 67175560

The run used real corpus WAV descriptors, bounded 16 KiB heads, the production page-intent worker, the immutable render model, and callback-side activation.
