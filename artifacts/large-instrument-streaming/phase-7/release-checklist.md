# Large-instrument streaming release checklist

Result: APPROVED

- [x] Debug and Release VST3 targets build.
- [x] Accurate Salamander: 641 WAV files, 641 projected sources, 1,704 projected zones.
- [x] Selected-zone and selected-group preparation are scoped and audible.
- [x] Full draft prepares in 443,977 us with 10,502,144 resident-head bytes and zero decoded full-corpus PCM bytes.
- [x] Normal sustained zone/group playback has zero page misses and zero underrun frames.
- [x] Maximum measured callback is 273 us within the 5,333 us budget.
- [x] Constrained 75 ms service polling degrades to bounded silence/miss counters without failure, wait, hang, or crash.
- [x] v2 export produces 40,865 independently addressable records with bounded 64 KiB working storage.
- [x] v2 structural verification, metadata open, warm reopen, head preparation, first note, and actual-corpus cancellation pass.
- [x] Package corruption, authentication, checksum, truncation, duplicate, oversized-record, sparse-offset, and cancellation matrices pass.
- [x] Standalone and plug-in open, activate, stream background pages, play, restore, and tear down with zero realtime violations.
- [x] Host `setStateInformation()` is nonblocking and package locator preparation is deferred.
- [x] v1 compatibility is limited to 64 MiB; oversized v1 receives explicit re-export guidance.
- [x] Compatibility, recovery, diagnostics, architecture, and operator documentation is current.
- [x] Production source audit finds no unbounded whole-corpus buffer in the qualified v2/WAV path.
- [x] The licensed derived package artifact is deleted after all real-package validation.

Evidence: `accurate-salamander-qualification.md`, `package-corruption-matrix.md`, `realtime-lifecycle-soak.md`, `host-qualification.md`, and `source-pattern-audit.md`.
