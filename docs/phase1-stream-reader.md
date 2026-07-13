# Phase 1 Stream Reader

This note captures the first Sprint 3 slice: a product-owned runtime reader for compiled `.drstrm` artifacts.

## Current scope

The stream reader now has three responsibilities:

- parse the compiler-emitted prototype `.drstrm` descriptor into product-owned runtime structs
- validate payload layout, prefetch heads, page-table entries, and source checksums
- resolve sample-relative read offsets either into the prefetch head or a page-table span

This is still a metadata reader, not a playback scheduler. It does not yet issue asynchronous disk reads or own page-cache lifetime. That comes in later Sprint 3 slices.

## What it validates

The reader currently checks:

- schema identity and version
- sample count and total payload size
- source file existence and checksum match
- payload-offset alignment
- prefetch size versus payload size
- expected page count from payload size, prefetch size, and page size
- exact page offsets and page sizes for each sample

Those checks are intentionally strict because Sprint 3 needs corruption to fail loudly before the runtime starts issuing real streaming reads.

## Read resolution seam

The reader also exposes a simple read-resolution helper that answers:

- is a requested payload-relative offset still inside the prefetch head?
- if not, which page-table entry owns it?
- what absolute byte offset and contiguous readable span does that imply?

That gives later scheduler and voice-state work a tested seam to build on without re-deriving the same page math in multiple places.

## Validation

`drs_phase1_stream_reader_tests` now proves:

- the checked-in reference `.drstrm` fixture loads cleanly
- instrument-zone stream offsets line up with stream-sample payload offsets
- page lookup resolves correctly in both prefetch and page-table ranges
- checksum corruption is detected safely
- page-offset corruption is detected safely

The shared `drs_phase1_pipeline_report` artifact now also includes a `streamReader` section so CI exposes stream-container health alongside loader, importer, compile, and corruption status.
