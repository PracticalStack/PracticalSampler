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
