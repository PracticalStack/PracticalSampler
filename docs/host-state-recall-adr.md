# Host-State Recall Architecture Decision

Status: Accepted  
Decision date: July 29, 2026  
Scope: `drs.hostState` schema version 1 and the DAW plug-in restore lifecycle  
Task: HS-001

## Context

`PluginProcessor::getStateInformation()` currently writes only the strict Phase 1
`drs.presetState` payload. That payload intentionally excludes `.drsproj` content, paths,
document revisions, and prepared playback identity. Consequently, a newly constructed plug-in
restores controls into `buildInitialAuthoringProject()` instead of identifying and restoring the
project that authored the saved DAW session.

This decision extends host recall without changing the existing preset contract or weakening the
Sprint 4 real-time and Sprint 6 publish invariants.

## Decision summary

1. Host state is a new outer `drs.hostState` version 1 envelope. The existing
   `drs.presetState` version 1 object is nested unchanged.
2. The engine-adapter layer owns the schema types, strict codec, validation, limits, and
   deterministic binding digest. The plug-in processor owns capture/publication and submission of
   restore requests. A shared project-restore coordinator owns asynchronous resolution and
   recovery state.
3. Clean saved projects restore from a validated file binding. Dirty or never-saved projects also
   carry a bounded embedded project snapshot. Sample bytes, prepared data, voices, caches, and
   diagnostics are never embedded.
4. A locator is only a hint. Project ID plus a deterministic manifest digest are authoritative.
5. Restore is generation-tagged and asynchronous. Only an exact current project, revision,
   authored-content, macro-schema, and prepared-content identity can activate.
6. A project-bound restore that cannot prove identity is silent. An existing last-known-good
   activation may continue only when it is already proven to belong to the same project ID.
7. Invalid input is rejected atomically. Legacy raw preset payloads remain accepted but are
   explicitly classified as unbound to a project.

## Ownership

| Responsibility | Owner | Rule |
|---|---|---|
| Host-state data types and codec | `engine_adapter` | No JUCE, UI, filesystem, or processor dependency |
| Deterministic project binding digest | `engine_adapter` | Computed from the canonical project serialization |
| Latest serialized host-state publication | plug-in `Processor` | Immutable shared byte block, rebuilt outside audio |
| Incoming request generation and project resolution | shared restore coordinator | File I/O and validation never occur in a host callback |
| Authoring document replacement | `AuthoringSession` / document controller | One validated checkpoint is applied atomically |
| Playback preparation and activation | existing facade, publish controller, and activation slots | Preserve Sprint 6 identity and block-boundary rules |
| Recovery presentation and actions | immutable coordinator snapshot plus shell adapters | Editor lifetime never owns restore work |

The standalone shell may reuse the codec and coordinator, but DAW host-state capture remains a
plug-in processor responsibility.

## Frozen version 1 envelope

All listed top-level fields are required. Unknown top-level fields fail parsing. Nested
`presetState` validation remains owned by the existing strict preset codec.

```json
{
  "schemaName": "drs.hostState",
  "schemaVersion": 1,
  "presetState": {},
  "projectBinding": {},
  "authoringState": {},
  "publishedState": {}
}
```

### `projectBinding`

| Field | Required | Meaning |
|---|---|---|
| `projectId` | yes for project-bound state | Stable authored project identity |
| `manifestPath` | no | Last known UTF-8 `.drsproj` path; a locator hint, not identity |
| `manifestFileName` | yes for project-bound state | Filename used for bounded relocation attempts and UI |
| `manifestDigest` | yes for project-bound state | Deterministic `fnv1a64:` digest of canonical project serialization |
| `contentRootHint` | no | Last known project/content directory for one bounded candidate lookup |
| `portableRelativePath` | no | Relative locator usable only when a shell supplies a trusted base directory |

Paths are normalized lexically for comparison but retain platform-native case semantics. No
process-global recent-project path participates in resolution.

### `authoringState`

| Field | Required | Meaning |
|---|---|---|
| `revision` | yes | Current document revision |
| `savedRevision` | yes | Last saved revision; must not exceed `revision` |
| `dirty` | yes | Must equal `revision != savedRevision` |
| `projectSnapshot` | conditional | Canonical project JSON object, never a JSON-encoded string |

`projectSnapshot` is required when `dirty` is true or when no persisted manifest locator exists.
It is omitted for a clean saved project. This prevents a clean host chunk from becoming a second
copy of the saved project while guaranteeing recall for unsaved work.

Undo/redo history is not serialized. On checkpoint restore both stacks are empty and the restored
revision metadata is retained.

### `publishedState`

All fields are optional as a group: either the object represents no previously published
Performance state, or it contains the complete identity below.

- `revision`
- `projectGeneration`
- `authoredContentDigest`
- `macroSchemaDigest`
- `preparedContentDigest`

These fields are verification/checkpoint data only. Prepared payloads and sample data are rebuilt,
never serialized. A saved project may have a dirty draft revision newer than the published
revision.

## Deterministic digest

Version 1 uses the repository's existing `fnv1a64:` digest convention. The binding digest is
computed over `serializeRuntimeProjectManifest(project, manifestPath)` using its ordered JSON and
LF output. Supplying the actual manifest path causes resolved project paths to be serialized
relative to that manifest when possible, so moving a self-contained project directory without
changing its contents preserves the digest.

The binding digest answers “is this the same authored manifest?” It is distinct from:

- playback snapshot `authoredContentDigest`, which identifies the captured playable revision;
- `macroSchemaDigest`, which identifies the published macro contract; and
- `preparedContentDigest`, which verifies the rebuilt immutable playback payload.

FNV-1a is a deterministic change detector, not a security boundary. Host-state input is still
treated as untrusted and validated independently.

## Size and structural limits

Limits are checked with overflow-safe arithmetic before retaining or expanding input:

| Dimension | Version 1 limit |
|---|---:|
| Entire incoming or outgoing host-state payload | 2,097,152 bytes |
| Embedded canonical project snapshot | 1,572,864 bytes |
| JSON nesting depth | 64 |
| Generic string | 65,536 UTF-8 bytes |
| Path string | 32,768 UTF-8 bytes |
| ID, schema name, or digest string | 512 UTF-8 bytes |
| Project sample sources | 8,192 |
| Project zones | 65,536 |
| Project groups | 2,048 |
| Project macros | 128 |
| Project FX slots | 128 |
| Project routing buses | 128 |
| Project performance banks | 256 |
| Notes or issues in any one collection | 4,096 |

The existing project loader remains responsible for schema and semantic validation after these
host-state budgets pass.

## Capture and restore threading

### Capture

The message-thread-owned service path rebuilds a serialized immutable host-state byte block after
any relevant project, binding, document, preset, or publish identity change. The processor
atomically publishes `shared_ptr<const HostStateBytes>`.

`getStateInformation()` atomically loads that publication and copies it into the JUCE
`MemoryBlock`. It does not traverse mutable project/facade state, touch the filesystem, wait, or
prepare playback.

### Restore

`setStateInformation()`:

1. rejects null, empty, or over-2-MiB input;
2. copies the bounded bytes into an immutable request;
3. assigns a monotonic request generation; and
4. submits it to the restore coordinator.

Schema detection, JSON parsing, project file I/O, digest calculation, project loading, and playback
preparation occur after the host callback on owned non-audio work. A newer request supersedes older
work. Stale results cannot mutate document or playback state.

The audio callback performs only the existing bounded immutable activation exchange.

## Resolution order

1. If a required embedded snapshot exists and validates, use it as the authoritative draft.
2. Otherwise try the exact normalized `manifestPath`.
3. If a trusted portable base was supplied, try `portableRelativePath` below that base.
4. Try exactly `contentRootHint / manifestFileName`.
5. If no candidate validates, publish `NeedsLocation`.

Resolution never performs an unbounded or recursive disk search. Every candidate must load as a
valid project before identity comparison.

## Identity mismatch and recovery

| Condition | Result | User action |
|---|---|---|
| Candidate missing | `NeedsLocation` | Locate or retry |
| Candidate project ID differs | `IdentityMismatch` | Locate again; candidate is never rebound |
| Project ID matches but digest differs | `ContentChanged` | Explicitly accept changed content or locate again |
| Embedded snapshot fails validation | `Failed` | Preserve current valid state; no partial fallback |
| Project loads but samples are missing | `Degraded` or `Failed` per existing completeness contract | Repair content and retry |
| Unknown host-state major version | `Failed` | Preserve current valid state |

Accepting changed content creates a new verified binding digest and marks the host-state
publication dirty so the DAW can persist the updated envelope. It never rewrites the project file.

Locate and acceptance actions are typed coordinator commands. They remain available if the editor
opens after unattended host restore; no modal UI blocks host loading.

## Pending-audio policy

Once a valid project-bound request is accepted:

- a fresh processor deactivates the startup/reference Performance payload and remains silent until
  matching restored content activates;
- an existing active payload may continue only if its stored project ID equals the requested
  project ID;
- identity mismatch, missing content, and changed content never cause the startup/reference
  instrument or an unrelated prior project to be presented as restored audio;
- failed, canceled, superseded, stale, or partial preparation cannot activate; and
- the exact previous same-project last-known-good payload may survive a failed newer publish,
  preserving the Sprint 6 contract.

## Preset application order

The nested preset is parsed and staged with the restore request. It is not committed to the
facade/host parameter mirror until the matching project manifest and macro schema are available.
Preset validation failure rejects the complete host-state restore atomically. Parameter
synchronization uses the existing host-notification path and must not create automation feedback.

## Contextual articulation restore freeze (HSR Sprint 1)

`drs.hostState` version 1 remains the persisted envelope. No schema migration is required merely
because a field is interpreted in the context of the restored authored project. The nested preset
continues to retain its existing strict schema and compatibility rules.

For a project-bound host restore, validation and application ownership is frozen as follows:

| Restored field | Validation/apply owner | Rule |
|---|---|---|
| Preset schema, target identity, and legacy compatibility | Reference preset contract | Keep the current strict preset codec and compatibility checks. |
| Load profile | Runtime load-profile registry | Require the saved profile to be known; do not infer or substitute a profile. |
| Published host macro values | Published host topology plus restored authored bindings | Validate host slots and authored ranges after the project checkpoint is available. |
| Selected articulation | Runtime instrument projected from the validated restored checkpoint | Require the saved articulation ID to exist in that authored project, not in an unrelated reference instrument. |
| Project checkpoint | Project-restore coordinator and document checkpoint validator | Validate project identity, binding digest, revision, and document constraints before any state is committed. |
| Published identity | Existing publish controller | Rebuild and retain the exact generation, revision, authored, macro-schema, and prepared-content identity gate. |

The Sprint 1 inventory identified two reference-only calls that made project-bound articulation
recall fail. The hosted path now calls the typed `validateProjectPresetState()` and
`restoreProjectPresetState()` facade contract. That contract retains strict reference-owned
schema, target, and load-profile validation while projecting articulations and authored macro
ranges from the restored project. It applies the validated state without reinitializing the
reference draft playback contract. Ordinary reference preset loading and legacy raw
`drs.presetState` restore continue to use the unchanged reference-owned path.

The `host-state-default-articulation.drsproj` regression fixture proves the corrected ownership:
its only authored articulation is `default`, whereas the Phase 1 reference fixture accepts
`sustain`. A valid project-bound capture must now reach `Active` and render finite, nonzero audio.
Changing that saved ID to the project-absent `sustain` must fail as `ArticulationMismatch` and
remain silent.

## Failure recovery and actionable diagnostics (HSR Sprint 2)

Project-bound application failures are terminal and typed. Checkpoint, binding, preset,
articulation, draft-playback, publish-scheduling, and published-identity failures remain separate
restore findings; they must never be collapsed into `CheckpointInvalid`. In particular, an
articulation mismatch reports the saved articulation ID, restored project ID, and the authored
articulation inventory including its default. The recovery banner presents that exact immutable
message off the audio thread.

After every terminal failure, the pending host-restore silence gate is cleared without activating
mismatched content. An explicit `replaceAuthoringProject()` or accepted manual Performance publish
supersedes the failed generation, clears stale expected identity state, and can activate a valid
manual project in the same plug-in instance. This policy does not weaken project binding or
published-identity checks, and `processBlock()` remains limited to its existing activation and
render exchange.

## Compiled VST3 and REAPER qualification (HSR Sprint 4)

`drs.host_state.vst3_qualification` is a host-side Debug test of the compiled VST3 bundle. It
discovers and instantiates the bundle through JUCE's VST3 host, wraps the captured processor
state in the VST3 `IComponent` stream that an actual host persists, restores it, renders MIDI,
and verifies finite nonzero samples plus the reserialized `drs.hostState` project binding. This
prevents a raw processor chunk from being mistaken for a VST3 component-state test.

`validation/reaper/run-host-state-qualification-matrix.ps1` creates a separate REAPER resource
directory and temporary project for each 44.1/48 kHz and 128/256/512-frame combination. It runs
both editor-open and editor-closed saved-reopen scenarios, inserts a MIDI note, and requires
enabled/online plug-in state, nonzero peak observations, and no non-finite observations. The
runner refuses to start if REAPER is already open: REAPER can forward command-line projects and
scripts to an existing user session, which is not isolated validation. Its `-cfgfile` argument is
the full path to the isolated `reaper.ini`; REAPER treats that file's parent as the alternate
resource directory. The timeout includes first-run resource initialization and plug-in scanning.

`validation/reaper/run-piano-lite-qualification-matrix.ps1` captures a newly published state from
the actual user Piano Lite project, first proves that state reaches `Active` with nonzero processor
audio, injects it into editor-open and editor-closed REAPER projects, and applies the same 44.1/48
kHz by 128/256/512-frame host matrix. Its evidence is isolated under
`validation/reaper/piano-lite-qualification-evidence`.

The compiled-VST3 Debug test and project-owned `default` articulation processor regression pass on
this workspace. The actual Piano Lite matrix was run on 2026-08-03 with all REAPER sessions closed:
all 12 editor-state, sample-rate, and block-size combinations produced finite, nonzero audio. The
signed evidence files are retained under `validation/reaper/piano-lite-qualification-evidence`.

## Legacy migration

A raw valid `drs.presetState` version 1 payload remains accepted:

- preset values restore under the existing validation contract;
- no project identity is inferred;
- presentation state records `LegacyUnboundProject`; and
- the next host save writes `drs.hostState` version 1 using the then-current project binding.

Invalid legacy payloads retain the existing last-known-good behavior. The legacy path is
compatibility behavior, not evidence of project-aware recall.

## Consequences

- Host chunks stay bounded and do not duplicate sample assets.
- Unsaved authored work can survive DAW save/reopen.
- Moved content is recoverable without silent substitution.
- Restore becomes asynchronous and independent of editor lifetime.
- Project loading and state application require a coordinator and explicit presentation states.
- Clean saved recall depends on the referenced `.drsproj` remaining available or being relocated
  by the user.

## Acceptance audit for HS-001

The previously open policy choices are now frozen:

- envelope ownership: decided;
- dirty and never-saved snapshots: decided;
- exact size/collection limits: decided;
- locator portability and bounded resolution: decided;
- digest algorithm and meaning: decided;
- mismatch and changed-content behavior: decided;
- pending-audio and last-known-good behavior: decided;
- host callback threading: decided;
- preset application order: decided; and
- legacy migration: decided.

Changes to these decisions require an explicit ADR revision plus updated fixtures and tests.
