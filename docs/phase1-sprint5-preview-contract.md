# Mini Sprint 5.1 Preview Contract And Behavior Baseline

Frozen July 19, 2026. This is the authoritative product and ownership contract for Sprint 5
Authoring Preview work. Later slices may replace temporary implementation seams, but they must not
change these decisions without an explicit contract revision and updated tests.

## Outcome

Preview auditions either the selected zone or the current unsaved draft through the isolated
Sprint 4 Preview playback context. Preview may prepare and activate immutable Preview models; it
must not publish, replace, reset, release, steal, or otherwise mutate Performance state.

## Current seam audit

| Current seam | Owner today | Disposition |
| --- | --- | --- |
| `Processor::serviceMessageThreadWork()` observes authoring revision and selected-zone changes. | Processor message path | Replace with the Sprint 5 Preview controller in 5.2. Selection identity must remain independent from document revision. |
| `Processor::stageAuthoringPreviewActivation()` formerly built an immediate selected-zone payload when worker state was stale. | Resolved in 5.4 | General authored worker payloads now cross one validating scope-preparation boundary. |
| `ensureSelectedAuthoringSampleLoaded(false)` formerly imported and cached the selected playback sample. | Resolved in 5.4 | Removed; worker-owned general authored preparation is the only playback decode path. |
| `queueAuthoringPreviewNoteOn()` formerly called `serviceMessageThreadWork()` implicitly. | Resolved in 5.5 | Typed audition commands request preparation explicitly; plain note dispatch is lifecycle-free. |
| Preview state formerly derived `Idle / Preparing / Ready / Stale / Failed` strings from processor diagnostics. | Resolved in 5.7 | UI and diagnostics now consume one typed immutable presentation snapshot; strings are presentation-only. |
| Summary Preview formerly used a detached delayed note-off while other authoring sources entered the processor queue directly. | Resolved in 5.5 | All creator sources use one typed adapter, per-source note ownership, and component-owned release timers. |
| Sprint 4 context activation, old-model voice leases, and message-owned retirement. | `SamplerPlaybackContext` | Retain unchanged. This is the approved activation/lifetime mechanism. |

All replacement seams are now retired. The former expected-red audit is the permanent registered
green `drs.sprint5.preview_contract_seams` regression target.

## Preview scopes

### Selected-zone Preview

Captures request ID, cancellation generation, current draft revision, selected zone ID, source and
route identity, articulation/group metadata, root/key/velocity range, gain, pan, sample start, loop
range, applicable macro values, and immutable prepared sample handles. It requires a selected zone
and filters to that audition route only after validating the source draft topology.

### Current-draft Preview

Captures request ID, cancellation generation, current draft revision, every Preview-eligible zone
and route, macro values, routing metadata required by the Sprint 4 render model, and immutable
prepared handles. It does not require a selected zone and never changes the published Performance
revision.

Selection ID and scope are first-class request identity. Two requests with the same draft revision
but different selection or scope are not equivalent.

## Typed lifecycle model

Preparation and activation are separate facts so one visible state cannot hide another:

- preparation: `idle`, `queued`, `preparing`, `ready`, `failed`, `canceled`, `superseded`;
- activation: `noActivation`, `pending`, `active`;
- presentation: `idle`, `queued`, `preparing`, `ready`, `activating`, `active`, `stale`, `failed`,
  `canceled`, `superseded`.

Ordinary preparation is `idle -> queued -> preparing -> ready`. Queued or preparing work may become
canceled or superseded. Preparing work alone may fail. Terminal work returns to idle or a new queued
request; it may not revive or become ready. Activation eligibility belongs only to the newest ready
request whose request ID, cancellation generation, revision, scope, and selected-zone identity still
match.

`stale` is a presentation fact: an older valid activation remains usable while the current request
is absent, pending, or failed. The status surface must expose current, requested, active, and failed
identities independently.

## Audition and active-note policy

| Event | Frozen behavior | Performance effect |
| --- | --- | --- |
| Summary Preview | Selected-zone request, then note-on when an eligible activation is available; paired note-off follows the owning gesture/timer. | None. |
| Authoring keyboard | Selected-zone by default; current-draft mode may route by full draft when explicitly selected. Exact note/velocity enter Preview only. | None. |
| Zone-map or inspector audition | Select/request the identified zone before note-on; the gesture owns note-off. | None. |
| Activation replacement | Voices already using the old immutable model finish or release against that model; new notes use the new activation after the block boundary. | None. |
| Selection change | Existing Preview voices finish; subsequent audition targets the new selection. Selection identity may supersede work without a draft revision change. | None. |
| Draft becomes stale | Existing voices finish. New audition uses last-known-good only when the UI explicitly identifies the audible stale revision; otherwise it waits for ready or yields deterministic silence. | None. |
| Preview stop | Ordinary all-notes-off release for Preview voices only. | None. |
| Project close | Immediate Preview reset, cancel queued/running requests, clear Preview activation through message-owned retirement. | None. |
| Device restart | Reset Preview voices but preserve/rebind the last usable immutable activation, matching the Sprint 4 context contract. | None. |

No-selection selected-zone requests are ineligible. Current-draft requests remain eligible without a
selection. Missing or invalid content never displaces last-known-good.

## Behavior baseline matrix

| Case | Required request/result behavior | Required audio/status behavior | Primary future slice |
| --- | --- | --- | --- |
| Summary Preview | Explicit selected-zone request; no hidden synchronous preparation in note-on. | Paired Preview note ownership; Ready/Active identity visible. | 5.2, 5.5 |
| Authoring keyboard | Typed source, exact note/velocity, selected/current-draft scope. | Sample-accurate Preview-only note-on/off. | 5.5 |
| Zone-map/inspector | Selection and request identity updated even at unchanged revision. | Old voices finish; new note targets new route. | 5.3, 5.5 |
| Mapping/gain/pan/root/range/offset/loop edit | New revision/signature; coalesced and prepared outside audio. | Old activation is Stale until newest eligible activation applies. | 5.3, 5.4 |
| Source relink/replacement | Correct prepared cache invalidation and immutable provenance. | Last-known-good remains audible until replacement is ready. | 5.4, 5.6 |
| Invalid/missing source | Structured failed result cannot activate. | Failed plus audible stale identity or deterministic silence; actionable guidance. | 5.6 |
| Rapid edits | Bounded pending/running work; obsolete work canceled/superseded. | Only newest eligible result activates; no stale flash. | 5.3 |
| Preview stop | No preparation side effect. | Release Preview only. | 5.5 |
| Project close/reopen | Cancel and detach old Preview; new project starts new request generation. | Reset/retire Preview only; Performance unchanged. | 5.3, 5.6 |
| Editor close | Controller/runtime work cannot depend on editor lifetime. | Audio and immutable diagnostics remain coherent. | 5.7, 5.8 |

## Thread and ownership rules

- The message thread owns controller state, request generation, coalescing deadlines, UI commands,
  result acceptance, activation staging, and presentation strings.
- Workers own validation, path resolution, decode/cache work, immutable snapshot/prepared assembly,
  cancellation observation, and structured result creation.
- The audio callback owns only bounded Preview event consumption, primitive activation exchange,
  rendering, voices, counters, and retirement-token production.
- UI readers consume immutable snapshots. They never inspect controller, worker, payload, activation
  slot, event queue, or voice storage directly.
- Performance is read-only to all Sprint 5 Preview work.

## Explicit non-goals

Sprint 5 does not define Apply/Publish commands, Performance preparation acceptance, Performance
cutover, held Performance-note transition, automation/macro migration between published revisions,
streaming/page integration, state recall, or final fixture removal.
