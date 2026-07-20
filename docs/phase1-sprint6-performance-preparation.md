# Mini Sprint 6.3 Full-Project Immutable Performance Preparation

Completed July 19, 2026. This slice makes a Performance Publish completion prove that it represents
the whole exact captured project before the existing activation contract may stage it.

## Preparation boundary

`PlaybackSnapshotBuilder` captures the complete project topology: all authored sample identities,
zones and their key/velocity/gain/pan/start/loop values, group and articulation routes, FX/routing
metadata, macro definitions/targets, and the exact draft revision. `PreparedPlaybackService` keeps
source fingerprinting, WAV/FLAC decoding, cache lookup/fill, decoded-data ownership, and prepared
handle construction on its worker boundary. Publish does not derive a selected-zone subset and the
completion validator performs no filesystem access, decoding, cache mutation, or document reads.

## Immutable conformance

The worker adds three deterministic digests to `ImmutablePreparedPlayback`:

- route digest over authored and prepared topology normalized by stable authored IDs;
- source-provenance digest over canonical source identity, byte fingerprint, format, rate, frames,
  and channels, normalized by sample-source ID; and
- macro-schema digest over stable macro IDs, ranges, and stable target identity.

These complement the authored snapshot and prepared-content digests. All five identities are carried
through the typed request/result, immutable activation payload, prepared revision, performance
snapshot, and diagnostics snapshot. Cache ownership tokens are intentionally excluded from the new
conformance digests, so cold and warm preparation agree.

`validatePerformancePublishPreparation()` independently recomputes every digest and checks exact
request/project/revision/snapshot/build linkage, one-to-one source/sample/stream/ownership coverage,
one-to-one authored/prepared zone coverage, handle bindings, source provenance, decoded-data
availability, and sample/loop bounds. Results are eligible only if the complete check has no error.

## Failure behavior

Every detected error has a stable code and path. Missing or invalid authored sources/formats remain
worker findings; the conformance boundary adds exact revision, digest, coverage, identity, handle,
route, provenance, range, cancellation, and partial-project findings. On any failure the facade gives
the draft playback contract an explicitly failed result, so no partial activation payload exists and
the last-known-good Performance revision remains untouched.

## Deferred boundaries

- Mini Sprint 6.4 owns final cross-lane priority, cooperative in-flight cancellation, and budgets.
- Mini Sprint 6.5 owns removal of processor-side activation eligibility/staging compatibility.
- Mini Sprint 6.7 owns immutable published macro value migration and host binding.
- Mini Sprint 6.8 owns typed shell presentation and direct-call removal.
