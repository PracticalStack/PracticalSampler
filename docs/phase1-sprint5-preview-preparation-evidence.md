# Mini Sprint 5.4 Completion Evidence

Completed July 19, 2026.

## Outcome

Selected-zone and current-draft Preview now consume the canonical general-authored worker payload.
The processor-local decode/cache and synthetic immediate activation path have been deleted. Equivalent
requests produce deterministic scoped digests and models, and Preview preparation leaves Performance
identity unchanged.

## Implemented artifacts

- Public deterministic immutable snapshot digest helper.
- `AuthoringPreviewPreparationResult` and `prepareAuthoringPreviewRenderModel`.
- Full-topology validation before selected-zone filtering.
- One-route immutable selected-zone payload derivation with shared decoded ownership.
- Full current-draft model preparation with authored macro/routing metadata intact.
- Processor/controller asynchronous worker cutover and explicit selected/current scope wiring.
- `drs.sprint5.preview_preparation`, registered with CTest and `drs_all_tests`.

## Deterministic validation matrix

The 5.4 target covers real external WAV and FLAC sources, cold and warm preparation, selected and
full scopes, one-of-three route retention after full validation, deterministic repeated identity,
off-route duplicate topology rejection, every required zone edit field, source relink, same-path
replacement, cache hit/miss behavior, immutable PCM ownership, provenance, digests, revision, and
Performance non-publication.

## Validation results

| Validation | Result |
| --- | --- |
| Sprint 5 focused matrix | **Passed 5/5** in 7.78 s. |
| `drs.sprint5.preview_preparation` | Passed in 4.43 s. |
| Inherited Sprint 4, entry-gate, and realtime-safety matrix | **Passed 14/14**; no regression. |
| Direct expected-red audit | Expected exit 1 with exactly two later-sprint seams. |
| Plugin shell compile | Passed after processor cutover. |

## Exit decision

Mini Sprint 5.4 exit criteria are met. Preview no longer depends on processor-local playback decode,
the checked-in fixture path, duplicate normalization, or an incomplete payload. P4 Draft Prepared is
passed. Mini Sprint 5.5 may proceed with typed audition commands and Preview-only event routing.
