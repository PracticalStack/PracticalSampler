# Mini Sprint 6.9 Completion Evidence

Date: July 20, 2026  
Decision: Pass

## Implemented evidence

- Replaced the historical expected-red seam executable with registered permanent green
  `drs.sprint6.publish_contract_seams` coverage.
- Removed the public `publishedRevisionState` lifecycle string and duplicate facade-owned published
  macro table; UI text now derives from typed immutable presentation state.
- Made the Publish controller retain the active immutable authorization payload after exact audio
  acknowledgement, including the revision-bound macro binding table.
- Preserved controller-owned Performance state across same-project authoring refresh while explicit
  project close/reopen and project-identity replacement clear it according to the frozen contract.
- Added `PerformancePublishIntegrationBudgets` and enforced every supported latency, work-depth,
  memory, retirement, voice, drop, callback, and diagnostics bound in the closure target.
- Added the closure target to `drs_all_tests` and the Linux ThreadSanitizer workflow.

## Measured result

One representative fresh Debug run recorded:

| Measure | Result | Limit |
|---|---:|---:|
| Request to active | 605,742 microseconds | 8,000,000 microseconds |
| Controller / worker pending depth | 1 / 1 | 1 / 2 |
| Worker in-flight / completed depth | 0 / 1 | 1 / 4 |
| Message-thread service | 200,401 microseconds | 250,000 microseconds |
| Retained activation bytes | 2,469,600 | 67,108,864 |
| Retirement backlog | 2 | 8 |
| Performance / Preview peak voices | 2 / 4 | 24 / 24 |
| Event/note drops | 0 | 0 |
| Callback overruns / realtime violations | 0 / 0 | 0 / 0 |

The controller-only failure/reorder scenario separately proves that a failed request preserves its
last-known-good payload and that an obsolete completion cannot replace a newer successful request.

## Validation matrix

| Gate | Result |
|---|---|
| Fresh Debug configure and aggregate build | Passed; `drs_all_tests` built from `build/sprint6-closure-debug` |
| Fresh Debug full CTest | **Passed 70/70** at `-j2` in 124.38 seconds |
| Sprint 6 focused closure matrix | Passed publish facade, seam, scheduling, macro, shell, integration, and realtime targets 7/7 |
| Sprint 4 and Sprint 5 inherited evidence | Passed within the full 70-test Debug matrix |
| Fresh Release VST3 build | Passed; `Decent Rhapsody Studio.vst3` produced |
| Release representative matrix | **Passed 7/7**: smoke, Sprint 4 offline, Sprint 5 closure, Sprint 6 closure, realtime, benchmark, and load |
| ThreadSanitizer coverage | Closure target registered in `.github/workflows/thread-sanitizer.yml`; execution is delegated to Linux CI |

## Defects found during closure

Three implementation gaps were corrected:

1. The facade retained a duplicate published macro table and a public compatibility lifecycle string.
   The controller now owns the activation payload and all lifecycle presentation is typed.
2. Explicit project close/reopen was accidentally preserving controller state while same-project
   authoring refresh needed preservation. The two cases now have separate, tested behavior.
3. Several inherited assertions still expected the retired lifecycle string. They now assert the
   typed preparation/presentation state appropriate to their activation boundary.

A full `-j4` Debug run passed 69/70 but forced the inherited realtime guard over its callback timing
budget while competing with CPU-heavy soaks. The same test passed alone, and the authoritative full
`-j2` matrix passed 70/70. No test is waived.

## Exit assessment

All Mini Sprint 6.9 tasks and gates A1-A9 are satisfied. Publish is explicit and bounded; stale or
ineligible work cannot activate; failure preserves last-known-good; revision, payload, and macro
truth are coherent; voice generations retain immutable ownership; and Sprint 4/5 gates remain green.

## Post-closure Perform-keyboard correction - July 20, 2026

Creator testing found that a published single-key imported zone could remain silent from the Perform
tab. The published `motion` control changed the effective MIDI note before zone lookup, so the
visible root key moved outside its own narrow key range and the renderer dropped the event.

Route selection now uses the physical played MIDI note and physical gesture velocity first. The
effective published layer velocity remains a compatibility fallback, and pitch modulation applies
only after a route is selected. The Perform panel also no longer invokes the legacy facade Preview
audition path: it emits only the processor-owned Performance note-on/off callbacks.

The added single-key/high-velocity-zone regression first reproduced both route failures and now
requires non-silent output after an explicit Publish. The smoke harness also drives the real JUCE
keyboard mouse gesture in both standalone and plug-in editors and requires measurable rendered
audio. The Perform surface now reports `Audio inactive` when no recent processor callback exists,
separating stopped-device/host processing from publication or routing failures. Focused Debug macro,
smoke, shell-routing, closure, realtime, and UI tests pass 6/6. The rebuilt Release standalone and
VST3 passed smoke, macro audibility, shell parity, full Sprint 6 integration, and realtime safety
5/5. The paced closure soak again passes its original budgets with no waived assertion.

A follow-up creator retest exposed a separate activation-boundary defect. The authored project
prepared successfully, but the processor carried the reference instrument's selected articulation
into the new immutable payload. When that stale id was absent from the authored routes, render-model
construction selected zero zones and failed with `performance-render-model-rejected`. Performance
activation now validates the selection against the payload's authored articulation routes, prefers
an authored `default` route, and otherwise chooses the first deterministic non-empty authored route.
Render-model rejection also preserves its specific structured finding instead of replacing it with a
generic activation message.

The status surface now renders a concise red `Publish Failed` chip rather than a green, clipped
combined diagnostics line. Full guidance and the finding code remain visible beside the keyboard
and through tooltip/accessibility descriptions. A regression deliberately publishes a single-key,
high-velocity project whose authored `default` articulation differs from bootstrap state and requires
active, non-silent Performance output. Focused Debug and rebuilt Release smoke, macro/audibility,
shell parity, integration, realtime, and Performance UI gates pass 6/6.
