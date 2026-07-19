# Sprint 4 Entry Gate: Immutable Activation Payload Contract

This document freezes the EG2 ownership boundary between worker completion, product state, and
future Sprint 4 renderer contexts.

## Renderer-facing payload

`PlaybackActivationPayload` is a const, shared, product-owned object containing:

- Preview or Performance lane identity;
- draft revision, snapshot build id, and prepared build id;
- activation eligibility and lane lifecycle state;
- snapshot and prepared-content digests;
- a shared const `ImmutablePlaybackSnapshot`;
- a shared const `ImmutablePreparedPlayback` whose sample handles retain decoded PCM;
- the logical prepared bytes retained by the payload.

Creating the payload copies immutable descriptor vectors but does not copy decoded PCM. Prepared
sample data remains shared through `PreparedPlaybackDecodedSampleData` handles.

## Installation rules

The DraftPlaybackContract retains separate last-known-good Preview and Performance payloads. A new
payload is installed only when snapshot and prepared results are both successful, activation
eligible, and agree on revision, snapshot build id, snapshot digest, and prepared digest.

Failed, canceled, superseded, stale, and identity-mismatched work cannot replace an existing
payload. The existing pointer, revision, digests, lifecycle eligibility, and prepared handles stay
active while the rejected work contributes actionable findings. Project close and a failed device
restart explicitly release both product-owned payloads. A successful device restart preserves
them.

## Block-boundary handoff

The processor uses four preallocated activation slots. The message owner fills an inactive slot
and publishes only its integer index. At the block boundary, the audio callback exchanges pending
and active indices and writes the retired index to a bounded single-producer/single-consumer ring.
The callback does not copy or destroy a shared payload.

Performance voices acquire a primitive lease on their activation slot. Retired slots with live
voice leases move to a fixed-capacity message-owned deferred list so they do not head-of-line block
unrelated reclamation. The message owner resets the final shared payload only after the last lease
is released. Large payload destruction therefore remains off the audio callback.

## Metrics and accounting

Facade performance and diagnostics snapshots expose Preview, Performance, and combined retained
activation-payload bytes. Processor real-time status distinguishes:

- active activation payload bytes;
- pending activation payload bytes;
- retired payload bytes awaiting non-audio cleanup;
- retired activation backlog;
- reclaimed activation payload count.

Payload byte values describe logical context retention. They are intentionally reported separately
from prepared-cache residency because both owners may share the same decoded PCM backing; they
must not be added together as physical memory consumption.
