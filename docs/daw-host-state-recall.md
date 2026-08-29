# DAW Host-State Recall

This is the shipped-behavior guide for `drs.hostState` version 1. The design rationale and
frozen contract are recorded in [host-state-recall-adr.md](host-state-recall-adr.md).

## What a DAW save contains

The plug-in publishes a bounded JSON envelope with four sections:

- `presetState`: the existing strict `drs.presetState` payload;
- `projectBinding`: project ID, manifest locator hints, filename, and canonical manifest digest;
- `authoringState`: revision, saved revision, dirty flag, and a conditional embedded project
  snapshot; and
- `publishedState`: the last published revision, project generation, authored-content digest,
  macro-schema digest, and prepared-content digest.

Sample bytes, stream assets, decoded buffers, caches, voices, diagnostics, undo history, and
background-worker state are never embedded. The maximum complete host chunk is 16 MiB and the
maximum embedded project snapshot is 15 MiB. This bounded limit admits the qualified 1,700-route
Salamander schema-7 project, whose routes each retain an immutable 128-point damper curve and
explicit zone pan field; sample and stream bytes remain excluded.

Clean saved projects use the validated `.drsproj` file. Dirty or never-saved projects carry a
bounded canonical project snapshot so authored changes survive a DAW save without duplicating
sample data.

## Capture and publish behavior

The message-thread service path creates an immutable serialized publication whenever the preset,
project binding, document revision, dirty state, or published identity changes.
`getStateInformation()` only copies that already-built byte block. It does not traverse mutable
authoring data, access the filesystem, wait for a worker, or prepare audio.

Saving a `.drsproj` validates and installs its project binding before the shell reports success.
Publishing Performance state adds its exact revision and prepared-content identity to subsequent
DAW chunks.

## Restore behavior

`setStateInformation()` bounds and copies the host bytes, assigns a monotonically increasing
generation, and submits asynchronous work. The processor owns a short-lived message-thread timer
while a host restore is in flight, so editor-closed instances continue through parsing, project
resolution, preparation, and activation.

Resolution is bounded:

1. validate a required embedded project snapshot;
2. try the exact manifest path;
3. try a trusted portable relative path when a base was supplied;
4. try `contentRootHint / manifestFileName`; and
5. enter `NeedsLocation`.

Every file candidate must match both the stable project ID and the canonical manifest digest.
No recursive disk search or process-global recent-project path is used.

The authored document is checkpointed and installed atomically. The nested preset is applied only
after the matching project and macro schema exist. Published audio is prepared by the existing
background pipeline and activates only when project generation, revision, authored-content,
macro-schema, and prepared-content identities all match.

## Recovery states

The plug-in and standalone shells show a compact, non-modal recovery banner:

| State | Meaning | Action |
|---|---|---|
| `NeedsLocation` / `CandidateMissing` | The saved locator is unavailable. | Locate the original `.drsproj` or restore it and retry. |
| `IdentityMismatch` | A selected project has a different project ID. | Locate the correct project; the candidate is never rebound. |
| `ContentChanged` | The project ID matches but canonical content differs. | Restore the saved version or explicitly choose the intended recovery workflow. |
| `PreparationFailed` | The project matched but playable content could not be prepared. | Repair missing/unsupported samples and retry. |
| `LegacyUnboundProject` | A raw version-1 preset restored without project identity. | Save the DAW session again to migrate to the host envelope. |
| `Active` | Project, preset, and published Performance identity are restored. | No action. |

Locate and Retry use the same identity verifier. Dismiss hides a notice for the current restore
generation only; a new failure generation is shown again.

## Audio policy

A fresh project-bound restore is silent until matching content activates. Missing, changed,
invalid, canceled, superseded, or stale work cannot expose the startup/reference instrument as
the recalled project. An already-active last-known-good payload may continue only when it is
proven to belong to the same project.

## Legacy migration

Raw `drs.presetState` version 1 remains accepted. Values restore under the existing preset
contract, no project is inferred, and the lifecycle reports `LegacyUnboundProject`. The next host
save emits `drs.hostState` version 1 using the currently validated project binding.

Unknown host-state versions, unknown fields, malformed JSON, invalid nested presets, inconsistent
revision metadata, over-budget input, and invalid embedded projects are rejected atomically.

## Limits and compatibility

- Host-state schema: `drs.hostState`, version 1.
- Preset schema: unchanged `drs.presetState`, version 1.
- Maximum input/output: 2,097,152 bytes.
- Maximum embedded project: 1,572,864 bytes.
- Maximum JSON depth: 64.
- Maximum path: 32,768 UTF-8 bytes.
- Maximum generic string: 65,536 UTF-8 bytes.
- Maximum zones: 65,536; sample sources: 8,192; groups: 2,048; macros: 128.

The manifest digest uses the repository's deterministic `fnv1a64:` convention. It is a content
change detector, not a security boundary.

## Troubleshooting

- If the banner says the project is missing, use Locate and select the original `.drsproj`.
- If it reports an identity mismatch, do not rename another project to the expected filename;
  locate the project with the matching embedded project ID.
- If it reports changed content, restore the saved manifest revision or deliberately resave the
  DAW project after reviewing the change.
- If preparation fails, open the authored project, repair sample paths or formats, prepare and
  publish Performance again, then resave the DAW project.
- If an older session reports `LegacyUnboundProject`, verify the intended project manually and
  save the DAW project once to migrate.
- For host-specific checks, follow [host-validation.md](host-validation.md) and compare captured
  evidence with [host-state-reaper-validation-evidence.md](host-state-reaper-validation-evidence.md).
