# Mini Sprint 6.1 Publish Contract And Entry Baseline

Frozen July 19, 2026. This is the authoritative product and ownership contract for Sprint 6
Performance Publish work. Later slices may replace the temporary implementation seams listed here,
but they must not change these decisions without an explicit contract revision and updated tests.

## Outcome

Performance changes only after a creator issues one typed Publish command, the message thread
captures one exact complete authored revision, workers prepare an immutable all-or-nothing payload,
and the eligible payload activates at one audio block boundary. Draft edits and Preview activity can
never publish implicitly. Failed, canceled, superseded, stale, partial, or mismatched work preserves
the exact last-known-good Performance payload and output.

## Entry baseline and replacement seams

| Temporary seam | Current owner | Sprint 6 disposition |
|---|---|---|
| `StatusPanel` directly calls `EngineFacade::publishCurrentDraft()` | Shared diagnostic UI | Replace with the typed Publish command adapter in 6.8. |
| Plug-in editor directly calls `EngineFacade::publishCurrentDraft()` | Plug-in shell | Replace with the shared typed adapter in 6.8. |
| Standalone shell directly calls `EngineFacade::publishCurrentDraft()` | Standalone shell | Replace with the shared typed adapter in 6.8. |
| Public `EngineFacade::publishCurrentDraft()` formerly owned request/result lifecycle | Resolved in 6.2/6.9 | It is an internal typed-adapter integration seam; direct shell access is retired and permanently audited. |
| Processor `stagePerformanceActivation()` owns eligibility/staging branches | Plug-in processor | Controller authorizes one immutable eligible payload in 6.5. |
| Public published lifecycle was exposed as `publishedRevisionState` text | Resolved in 6.8/6.9 | The compatibility string is deleted; UI text derives from typed immutable presentation state. |
| Mutable facade macro values were not bound to an immutable published schema | Resolved in 6.7/6.9 | The controller-owned active authorization payload carries the revision-bound immutable macro table. |

All replacement seams are retired. `drs.sprint6.publish_contract_seams` is the registered permanent
green regression target covering direct shell routing, typed lifecycle truth, controller payload
ownership, macro binding ownership, and the processor's bounded callback view.

## Command semantics

`PerformancePublishCommandType::publishCurrentDraft` is the only command that authorizes a
Performance change. The command carries creator intent, not mutable document content. The message
thread captures the request identity exactly once when accepting that command.

| Case | Frozen behavior |
|---|---|
| Dirty draft | Capture its exact current revision/content/macro schema; Performance remains last-known-good until activation. |
| Clean draft equal to active identity | Controller may suppress an exact duplicate without rebuilding or activating. |
| Repeated click while preparing | An exact duplicate collapses; a different captured identity supersedes the older request. |
| Edit during preparation | In-flight work continues or observes cancellation against its original immutable input; it never reads the newer mutable draft. |
| No/closed project | Reject with a typed actionable finding and preserve last-known-good according to project-lifecycle policy. |
| Invalid/missing content | Return a complete failed result; never stage a partial payload. |
| Editor closed | Controller, workers, activation, and status continue independently of editor lifetime. |

Selection changes and Preview requests are not Publish commands. Authoring macro edits remain
draft/Preview-only until an explicit Publish succeeds.

## Request and result identity

Full request identity is:

- monotonic request ID;
- cancellation generation;
- project generation;
- captured draft revision;
- authored content digest; and
- authored macro schema digest.

All fields participate in equality. A result is eligible only when its identity exactly equals the
current request, it represents the complete project, activation is explicitly eligible, prepared
build ID and content digest are nonzero/nonempty, and its prepared macro schema digest equals the
captured schema digest.

Any violation yields `preserveLastKnownGood`; it cannot yield partial staging, Ready, Pending
Activation, or Active.

## Typed lifecycle

Preparation and activation are independent facts:

- preparation: `idle`, `queued`, `preparing`, `ready`, `failed`, `canceled`, `superseded`;
- activation: `noActivation`, `pending`, `active`;
- presentation: `idle`, `queued`, `preparing`, `ready`, `activating`, `active`, `stale`, `failed`,
  `canceled`, `superseded`.

The ordinary preparation path is `idle -> queued -> preparing -> ready`. Queued and preparing work
may become canceled or superseded; preparing work may fail. Terminal work may return to idle or a
new queued request but may not revive directly to ready/preparing. A usable active identity remains
independent while a newer request is queued, preparing, failed, canceled, or superseded.

## Last-known-good and activation

Only an eligible complete result may be staged. Message-thread staging uses the Sprint 4 immutable
activation slots. The audio callback performs only a bounded block-boundary exchange and reports the
exact activated identity. Payload construction, validation, macro schema assembly, findings, and
large-resource destruction are forbidden on audio.

Failure state and findings never overwrite or relabel the active identity. Replaced, rejected, and
failed payloads retire away from the callback after all activation/voice leases release them.

## Voice-generation policy

Sprint 6 freezes old-generation completion as the default cutover policy:

- voices started before activation retain their original immutable generation;
- note-off routes to the generation that owns the voice;
- retriggers after activation use the new active generation; and
- Performance stop/all-notes-off releases every Performance generation without touching Preview.

Optional crossfade DSP is not a baseline requirement. It may be introduced only if Mini Sprint 6.6
produces evidence that immutable old-generation completion cannot satisfy a documented route case.

## Macro and automation migration

Host parameter topology remains stable. Published macro migration is based on stable authored ID,
never list position:

- compatible ID: preserve current Performance/host value and clamp into the new range;
- changed range: preserve and clamp by stable ID;
- added ID: use the new authored default;
- removed ID: retire the binding deterministically; and
- reordered ID: preserve by stable ID without changing its semantic value.

The immutable macro binding/value view activates at the same block boundary as its Performance
payload. Mixed old-schema/new-audio state is forbidden.

## Thread and ownership rules

- The message thread owns command acceptance, exact draft capture, request generation, controller
  transitions, result acceptance, activation staging, macro migration, and presentation text.
- Workers own full-project validation, source resolution, decode/cache work, deterministic immutable
  assembly, cancellation observation, and structured result creation.
- The audio callback owns only bounded Performance event consumption, primitive activation exchange,
  voice rendering/generation ownership, counters, and retirement-token production.
- UI readers consume immutable snapshots and issue typed commands. They never inspect mutable
  controller, worker, activation, payload, voice, or document state directly.
- Preview retains independent controller, event queue, activation, voice pool, ownership, and status.

## Explicit non-goals

Sprint 6 does not implement stream/page pressure, state save/recall identity, moved-content recovery
across sessions, device/sample-rate/block-size restart, suspend/resume, production profiling, final
fixture removal, or Phase 3 extension behavior. These remain assigned to Sprints 7-8.
