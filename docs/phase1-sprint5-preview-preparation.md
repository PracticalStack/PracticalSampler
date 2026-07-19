# Mini Sprint 5.4 Selected-Zone And Current-Draft Preparation

Completed July 19, 2026.

## Preparation boundary

`prepareAuthoringPreviewRenderModel` is the message-owned boundary between the general authored
Preview worker and the renderer. It accepts only a ready immutable Preview activation payload whose
revision matches the controller request. It first builds and validates the complete render model.
Selected-zone filtering happens only after that validation succeeds, so malformed off-route project
topology cannot be hidden by a narrow audition request.

Selected-zone preparation derives a new immutable payload retaining one snapshot route, its prepared
zone, sample, stream, ownership records, and applicable articulation/group references. Prepared PCM
ownership and source provenance remain shared; decoded channel storage is not copied. Snapshot and
prepared digests are recomputed for the scoped topology.

Current-draft preparation retains the complete validated worker payload: all Preview-eligible zones,
prepared sources, macro defaults, routing metadata, and authored handles. It never publishes or
replaces Performance.

## Processor cutover

The processor no longer imports or caches a selected sample for playback and no longer manufactures
synthetic snapshot, prepared, or activation payloads. Controller launch requests general authored
Preview preparation from `EngineFacade`; a later message-thread service pass accepts the matching
worker payload, normalizes it through the 5.4 boundary, and stages it in `SamplerPlaybackContext`.

The summary Preview action now requests `currentDraft`; keyboard note audition requests
`selectedZone`. Request fingerprints cover the full serialized authored project for current-draft
scope and selected route plus source path for selected-zone scope.

## Invalidation and ownership

Zone-only mapping, gain, pan, root, key/velocity bounds, start, and loop edits reuse warm decoded
sample ownership while producing a new route model. Source relinks and same-path file replacement
change the source fingerprint and cold-prepare only invalidated ownership. WAV and FLAC format name,
source/canonical path, canonical identity, fingerprint, cache key, ownership token, revision, digest,
and decoded data survive into the scoped activation.

Structured renderer findings cross the same boundary. Missing payload, wrong lane, revision mismatch,
missing selection, missing selected route, invalid prepared indices, and full-topology renderer
validation each have stable codes and paths.

