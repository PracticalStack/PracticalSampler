# Phase 1 Streaming Service

This note captures the second Sprint 3 slice: the first product-owned background streaming service that sits on top of the `.drstrm` reader.

## Current scope

The streaming service now provides:

- non-blocking page-read request submission
- one background worker that resolves queued reads off the requester thread
- cache hits for already resident pages
- coalescing for duplicate in-flight page requests
- explicit page leases plus release calls
- simple cache-budget enforcement through post-release eviction

The current service still uses synthetic page payload bytes derived from the stream descriptor metadata. That is intentional for this slice because the prototype `.drstrm` artifact is still a metadata contract, not a final binary container.

## Why synthetic bytes are acceptable here

Sprint 3 slice 2 is about scheduling and ownership, not final audio decoding.

The service therefore proves:

- request and completion happen on different threads
- rapid request bursts can queue without blocking the caller like synchronous I/O
- page residency and lease lifetime are explicit and measurable

Later slices can swap the synthetic payload fill for real compiled-container reads without rewriting the queueing, caching, or lease contract.

## Validation

`drs_phase1_streaming_service_tests` now proves:

- a first page request queues and resolves asynchronously
- duplicate in-flight requests coalesce
- ready pages produce cache hits
- completion happens on a worker thread rather than the requester thread
- rapid bursts of page requests drain in the background
- lease release enables cache eviction back down to the configured budget

The shared `drs_phase1_pipeline_report` artifact now also includes a `streamScheduler` section so CI exposes whether the background-read seam still settles correctly for the reference corpus.
