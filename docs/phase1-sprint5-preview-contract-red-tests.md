# Sprint 5 Preview Replacement-Seam Regression Audit

Recorded July 19, 2026.

## Purpose

`drs_sprint5_preview_contract_red_tests` made the temporary Preview orchestration gaps executable
through incremental implementation. Mini Sprint 5.7 removed the final gap and converted the target
into the registered green `drs.sprint5.preview_contract_seams` negative source regression audit.

## Expected failures

The executable inspects the current product sources and returns exit code 1 if any of these retired
replacement seams returns:

1. processor-owned immediate Preview payload construction;
2. synchronous selected-sample warming during Preview staging;
3. implicit message servicing from Preview note-on;
4. processor-owned Preview lifecycle synchronization (removed by Mini Sprint 5.2); and
5. string-only public Preview lifecycle state.

An unreadable source or audit failure returns exit code 2. With all five seams absent, the executable
returns zero as a permanent negative source audit.

## Resolution map

| Red seam | Resolution owner |
| --- | --- |
| Immediate payload construction and synchronous warming | Mini Sprint 5.4 |
| Implicit service from note-on | Mini Sprints 5.2 and 5.5 |
| Processor lifecycle synchronization | Mini Sprint 5.2 |
| String-only lifecycle state | Mini Sprint 5.7 |

## Mini Sprint 5.2 update

The processor-owned `synchronizeAuthoringPreviewActivation` path and its revision/selection/build
observation fields were removed. The direct audit now reports exactly four remaining seams. This is
the expected incremental red state; the other four checks stay active for their assigned slices.

## Mini Sprint 5.3 update

Bounded coalescing, cancellation signaling, completion records, and warm-result identity reuse are
implemented without changing the four remaining source seams. The audit is expected to remain red
at exactly four until Mini Sprints 5.4, 5.5, and 5.7 retire their assigned implementation gaps.

## Mini Sprint 5.4 update

The processor-owned immediate payload construction and synchronous selected-sample warming seams
were removed. The direct audit now reports exactly two remaining seams: implicit note-on message
servicing and string-only public presentation state. Their owners remain Mini Sprints 5.5 and 5.7.

## Mini Sprint 5.5 update

Plain Preview note-on no longer services controller or worker work. Selected-zone and current-draft
audition commands request preparation explicitly, while every note source enters the typed command
adapter. The direct audit now reports exactly one remaining seam: string-only public presentation
state, owned by Mini Sprint 5.7.

## Mini Sprint 5.6 update

Last-known-good identity, structured failure families, repair, resource pressure, and project
lifetime are implemented without replacing the presentation-state seam assigned to 5.7. The direct
audit therefore remains intentionally red at exactly one finding.

## Mini Sprint 5.7 update

The public Preview status now carries typed preparation, activation, and presentation states. The
UI consumes one atomically published immutable snapshot and uses strings only for creator-facing
labels and guidance. The final seam is removed; the audit is green and registered with CTest and
`drs_all_tests` as `drs.sprint5.preview_contract_seams`.

The green `drs.sprint5.preview_contract` target is registered immediately. It freezes request scope,
request identity, legal preparation transitions, and active-note policy while the red implementation
gaps are resolved incrementally.
