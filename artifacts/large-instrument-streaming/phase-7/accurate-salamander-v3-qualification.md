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
- Import analysis/projection: 434264 us
- Atomic authoring apply: 13225 us
- Full snapshot: 256055 us
- Selected-zone retained zones/sources: 6/6
- Selected-group retained zones/sources: 21/21
- Selected-zone preparation: 3697 us
- Selected-group preparation: 11801 us
- Full-draft preparation: 543086 us
- Estimated resident decoded bytes: 2613205608
- Full-draft decoded bytes: 0
- Full-draft resident-head bytes: 41746432
- Full-draft region prewarm intents accepted/rejected: 0/0
- Default middle-C eligible routes: 1
- Enabled middle-C auxiliary resonance routes: 2
- CC20 middle-C sampled-release routes: 2
- CC21 middle-C hammer routes: 1
- Retained random pedal-transition routes: 0
- Approved minimum-reference regions: 1408
- Standard/minimum reference peak: 0.185453/0.185453
- Standard/minimum reference RMS: 0.0298142/0.0298142
- Standard/minimum reference duration frames: 71758
- Standard/minimum maximum sample-aligned error: 0
- Standard/minimum RMS sample-aligned error: 0
- Standard/minimum maximum spectral-profile error: 0
- Standard/minimum strict parity enforced: yes
- Standard/minimum within strict tolerance: yes
- Selected-zone sustained peak/elapsed: 0.370679796/2592513 us
- Selected-group sustained peak/elapsed: 0.370679796/2602230 us
- Zone worker intents/prepared/failures: 3016/392/0
- Zone page cache bytes: 786432
- Zone maximum page read latency: 5856 us
- Zone maximum callback duration/budget: 600/5333 us
- Zone intent published/consumed/dropped/max-depth: 3016/3016/0/24
- Zone cache budget/peak/leased/retired bytes: 402653184/786432/0/0
- Zone page misses/underrun frames/recoveries: 0/0/0
- Group intents/prepared/failures: 3016/396/0
- Group page misses/underrun frames/recoveries: 0/0/0
- Constrained profile: 75 ms page-service poll
- Constrained peak/elapsed: 0.370679796/2544270 us
- Constrained intents/prepared/failures: 264/33/0
- Constrained page misses/underrun frames/recoveries: 637696/637696/0
- Peak process working set: 579149824 bytes

## Authenticated package v3

- Package bytes: 2623997763
- Frozen V2 package bytes: 2631961513
- V3 package overhead versus V2: -0.302578513%
- Records: 40783
- Export elapsed: 31209722 us
- Export throughput: 83930720 plaintext bytes/s
- Peak plaintext/sealed buffers: 65536/65536 bytes
- Structural verification bytes: 2628584622
- Metadata open: 8395509 us
- Warm metadata reopen: 8349677 us
- Head-ready/playable: 5310768 us
- Package resident-head bytes: 10436608
- Package first-note peak: 0.0463349707
- Package maximum page read latency: 12632 us
- Package maximum callback duration/budget: 84/5333 us
- Package sustained playback elapsed: 135870 us
- Package sustained page misses/underrun frames/recoveries: 0/0/0
- Actual-corpus cancellation latency/polls: 339322 us/26958
- Actual-corpus work-before-cancel plus response: 34927349 us
- Actual-corpus cancellation bytes processed: 67164044

The run used real corpus WAV descriptors, bounded 16 KiB source heads with a 64 KiB decoded-residency ceiling, the production page-intent worker, the immutable render model, and callback-side activation.
