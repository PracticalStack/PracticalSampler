# Phase 1 Sample Import Seam

This note captures the first Sprint 2 slice of the import pipeline. The goal of this seam is to stop compile-path work from depending on ad hoc audio decoding logic scattered across tools or UI code.

## Current scope

The product-owned importer now exposes a single entry point:

- `drs::engine::importSampleFile(...)`

That entry point currently does four things:

- decodes supported source audio through JUCE's audio-format readers
- normalizes decoded data into product-owned per-channel float buffers
- extracts baseline metadata needed by later manifest and compiler work
- reports actionable errors for missing, unsupported, or policy-rejected files

The Phase 1 policy rules that sit on top of decode success are documented in:

- `docs/phase1-import-policy.md`

## Metadata captured in Sprint 2 slice 1

- source path
- source format name
- source file checksum
- sample rate
- frame count
- channel count
- bit depth
- floating-point versus fixed-point source flag
- channel layout description
- duration in seconds
- optional root MIDI note when exposed by the source metadata
- optional loop start and end when exposed by the source metadata

## Validation

`drs_phase1_sample_import_tests` generates matched WAV and FLAC fixtures from the same in-memory buffer, imports both through the product-owned seam, and checks:

- equivalent normalized duration, frame count, sample rate, and channel count
- equivalent normalized sample data within tolerance
- WAV loop and root-note metadata extraction
- actionable failure reporting for missing and unsupported inputs
- policy warnings for non-portable sample names
- policy rejections for unsupported decoded formats, sample rates, and channel counts

## Why this is the first Sprint 2 slice

The compiler path should not own audio decoding. By making sample import a tested seam first, the next slice can focus on deterministic manifest and stream-container generation instead of also solving file-format I/O at the same time.
