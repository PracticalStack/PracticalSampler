# Performance Package Content Protection Baseline

Status: Accepted for implementation

Date: 2026-08-27

## Existing reference measurements

The existing UI responsiveness fixture records package and authored workflow
timings but is not a cryptographic benchmark. Current Debug evidence includes:

| Measurement | Existing value |
| --- | ---: |
| Authored Preview ready | 9.142 s |
| Publish ready after prepared Preview reuse | 328 ms |
| Package load | 138 ms |
| Maximum package audio block | 1.034 ms |

The large-instrument qualification records 2.6 GB package output, 10.5 MB
resident heads, 367 ms cold metadata open, 43 ms warm metadata reopen, and
117 ms head-ready/playable preparation. These values are comparison baselines,
not new V3 budgets.

## V3 budgets to approve before Phase 2 release

- Metadata open: no more than 10% above the same V2 fixture on a warm cache.
- Head-ready/playable: no more than 15% above the same V2 fixture.
- Page-service maximum latency: no more than 15% above V2.
- Peak resident/cache bytes: no increase attributable to unbounded crypto
  buffering; plaintext remains bounded to the existing page contract.
- Package-size overhead: record framing, nonce, tag, envelope, and signature
  overhead measured separately from content conversion.
- Audio callback: zero crypto, I/O, allocation, or signature work; no new
  callback guard violations.

## Measurement rules

Every comparison records build configuration, corpus, filesystem cache state,
package format, record count, package bytes, metadata time, head time, first
note, page latency, cancellation, peak memory, and sustained playback status.
Raw encrypted bytes are not used as a determinism criterion. Semantic runtime
digests and rendered output must remain stable across exports.
