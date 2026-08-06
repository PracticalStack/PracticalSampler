# Accurate Salamander large-instrument qualification

Result: PASS

Signed by: DRS automated large-instrument qualification (Release)

Corpus: `E:/Dev/Cpp/VST/DecentRhapsody/DemoSFVInstruments/AccurateSalamanderGrandPianoV6.2beta2_48khz24bit/sfz_daw/Accurate-SalamanderGrandPiano_flat.Recommended.sfz`

- WAV files: 641
- Corpus WAV bytes: 1964042398
- Projected sources: 641
- Projected zones/routes: 1704
- Import analysis/projection: 257420 us
- Atomic authoring apply: 7975 us
- Full snapshot: 60396 us
- Selected-zone retained zones/sources: 1/1
- Selected-group retained zones/sources: 17/17
- Selected-zone preparation: 1586 us
- Selected-group preparation: 7807 us
- Full-draft preparation: 443977 us
- Estimated resident decoded bytes: 2618648472
- Full-draft decoded bytes: 0
- Full-draft resident-head bytes: 10502144
- Selected-zone sustained peak/elapsed: 0.454940587/3060333 us
- Selected-group sustained peak/elapsed: 0.896996021/5771019 us
- Zone worker intents/prepared/failures: 3000/385/0
- Zone page cache bytes: 786432
- Zone maximum page read latency: 731 us
- Zone maximum callback duration/budget: 273/5333 us
- Zone intent published/consumed/dropped/max-depth: 3016/3000/0/20
- Zone cache budget/peak/leased/retired bytes: 67108864/786432/0/0
- Zone page misses/underrun frames/recoveries: 0/0/0
- Group intents/prepared/failures: 5528/717/0
- Group page misses/underrun frames/recoveries: 0/0/0
- Constrained profile: 75 ms page-service poll
- Constrained peak/elapsed: 0.454940587/5776665 us
- Constrained intents/prepared/failures: 592/74/0
- Constrained page misses/underrun frames/recoveries: 686704/686704/0
- Peak process working set: 225513472 bytes

## Package v2

- Package bytes: 2631961513
- Records: 40865
- Export elapsed: 21641510 us
- Export throughput: 121268906 plaintext bytes/s
- Peak plaintext/sealed buffers: 65536/65592 bytes
- Structural verification bytes: 5247332
- Metadata open: 367185 us
- Warm metadata reopen: 43487 us
- Head-ready/playable: 117347 us
- Package resident-head bytes: 10502144
- Package first-note peak: 0.0845512301
- Actual-corpus cancellation latency/polls: 6580817 us/1121
- Actual-corpus cancellation bytes processed: 67230177

The run used real corpus WAV descriptors, bounded 16 KiB heads, the production page-intent worker, the immutable render model, and callback-side activation.
