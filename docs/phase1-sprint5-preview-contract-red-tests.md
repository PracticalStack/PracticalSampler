# Mini Sprint 5.1 Expected-Red Preview Regressions

Recorded July 19, 2026.

## Purpose

`drs_sprint5_preview_contract_red_tests` makes the temporary Preview orchestration gaps executable
through the incremental controller implementation. It is intentionally not registered with CTest or
`drs_all_tests`; a known failing target must not contaminate the green regression baseline.

## Expected failures

The direct executable inspects the current product sources and returns exit code 1 while any of
these replacement seams remains:

1. processor-owned immediate Preview payload construction;
2. synchronous selected-sample warming during Preview staging;
3. implicit message servicing from Preview note-on;
4. processor-owned Preview lifecycle synchronization (removed by Mini Sprint 5.2); and
5. string-only public Preview lifecycle state.

An unreadable source or audit failure returns exit code 2 so an infrastructure problem cannot be
mistaken for expected red evidence. When all five seams are removed, the executable returns zero
and should be retired or converted to a permanent negative source audit.

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

The green `drs.sprint5.preview_contract` target is registered immediately. It freezes request scope,
request identity, legal preparation transitions, and active-note policy while the red implementation
gaps are resolved incrementally.
