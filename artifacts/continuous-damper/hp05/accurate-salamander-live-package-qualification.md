# Accurate Salamander live-preset package qualification

Result: PASS

Signed by: DRS automated large-instrument qualification, Release, August 15, 2026.

Corpus: `DemoSFVInstruments/AccurateSalamanderGrandPianoV6.2beta2_48khz24bit/sfz_live/Accurate-SalamanderGrandPiano_flat.Recommended.sfz`

## Corpus and projection

- WAV files: 641
- Corpus WAV bytes: 1,964,042,398
- Projected sources: 637
- Projected zones/routes: 1,700
- Semantically analyzed regions: 1,704
- Unsafe unconditional regions omitted: 4
- Import analysis/projection: 249,379 us
- Atomic authoring apply: 6,167 us
- Full snapshot: 70,773 us
- Full-draft preparation: 329,755 us
- Estimated resident decoded bytes: 2,613,205,608
- Full-draft decoded bytes: 0
- Full-draft resident-head bytes: 10,436,608

The live preset intentionally differs from the stripped `sfz_minimum` reference.
The qualification records that difference instead of applying the older strict
sample-parity assertion: peak 0.235427/0.235525, RMS 0.0379853/0.037864,
maximum sample-aligned error 0.212897, and maximum spectral-profile error
0.000415425. Both renders remained finite and audible.

## Streaming

- Selected-zone/group normal-storage page misses: 0/0
- Selected-zone/group underrun frames: 0/0
- Selected-zone maximum callback duration/budget: 739/5,333 us
- Page-worker prepare failures: 0
- Constrained 75 ms service profile: bounded misses and silence were reported;
  no wait, hang, or crash occurred.
- Peak process working set: 256,036,864 bytes

## Package v2

- Package bytes: 2,631,113,602
- Records: 40,846
- Export elapsed: 21,501,841 us
- Export throughput: 122,017,357 plaintext bytes/s
- Peak plaintext/sealed buffers: 65,536/65,592 bytes
- Structural verification bytes: 5,264,722
- Metadata open: 305,993 us
- Warm metadata reopen: 16,369 us
- Head-ready/playable: 74,797 us
- Package resident-head bytes: 10,436,608
- Package first-note peak: 0.0633019358
- Actual-corpus cancellation latency/polls: 36,538,697 us/22,047
- Actual-corpus cancellation bytes processed: 67,175,570

The run used real corpus WAV descriptors, bounded 16 KiB heads, the production
page-intent worker, the immutable render model, authenticated package reopen, and
callback-side activation. The 2.63 GB temporary qualification package was not
checked into source control.
