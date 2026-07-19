# Sprint 4 Entry Gate: General Authored Preparation Contract

This document freezes the EG1 worker input contract for Preview and Publish preparation.

## Eligibility and ownership

An accepted immutable playback snapshot is the eligibility boundary. Every snapshot sample is
resolved by `PreparedPlaybackService` on its worker path. Shell, editor, and audio-callback code
must not decode, fingerprint, open, or synchronously fall back to a source file.

Compiled Phase 1 stream metadata is optional. When a matching compiled stream sample exists and
its checksum equals the fingerprint computed from the current source bytes, preparation retains
its container and page topology. Otherwise, the worker decodes the authored file into immutable
normalized PCM and emits a lightweight `decoded-memory` stream binding so existing zone-to-stream
indices remain stable.

## Per-sample input identity

The worker resolves and records:

- immutable snapshot sample index and authored source id;
- lexically normalized canonical source path;
- fingerprint computed from the file bytes observed by the worker;
- decode policy (`decoded-float32`/`decoded-memory` for general authored inputs);
- optional compiled stream sample id, format, container, payload, and page topology;
- resolution kind: compiled path match, compiled id match, or authored source.

Reference-stream membership is never required for preparation eligibility.

## Cache identity

Prepared cache keys contain exactly these invalidation dimensions:

1. compiler-version salt;
2. canonical identity (`sampleSourceId|canonicalPath`);
3. actual worker-computed source fingerprint;
4. effective decode-policy fingerprint.

Zone-only edits do not invalidate decoded source assets. Relinking changes canonical identity and
cold-misses that source. Replacing bytes at the same path changes the fingerprint and cold-misses
that source. Unchanged sources remain warm. Stale compiled checksums disable compiled topology
rather than allowing stale prepared audio or rejecting the authored source.

## Structured failure vocabulary

- `prepared-sample-source-missing`: the source disappeared before worker realization.
- `prepared-sample-format-unsupported`: current bytes are not a supported audio format.
- `prepared-sample-decode-failed`: fingerprinting, decode, read, or import policy failed.
- `prepared-sample-stream-mismatch`: current decoded metadata conflicts with otherwise-current
  compiled topology metadata.
- `missing-prepared-zone-sample`: a zone cannot bind because its sample failed preparation.

Failures remain worker findings and flow through the frozen Preview or Publish revision together
with revision, snapshot digest, lifecycle state, prepared counts, cache hit/miss counts, retained
bytes, and build duration. Failed, canceled, and superseded work is not activation eligible.

## Background-worker startup

Background execution waits until a stream context has been explicitly configured. That context
may be a loaded compiled container or an intentionally empty result. This prevents startup races
while preserving container-free authored preparation.
