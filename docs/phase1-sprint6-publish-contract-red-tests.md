# Sprint 6 Publish Seam Audit - Historical Expected-Red Record

Date established: July 19, 2026  
Original target: `drs_sprint6_publish_contract_red_tests`

Permanent target: `drs_sprint6_publish_contract_seam_tests`

CTest registration: `drs.sprint6.publish_contract_seams`

## Purpose

The audit makes the temporary Publish implementation boundary executable. It reads the relevant
source files and returns exit 1 while any known replacement seam remains. This is expected evidence,
not a waived green test and not a product failure.

## Initial seven seams

1. `StatusPanel` directly invokes `EngineFacade::publishCurrentDraft()`.
2. The plug-in editor directly invokes `EngineFacade::publishCurrentDraft()`.
3. The standalone shell directly invokes `EngineFacade::publishCurrentDraft()`.
4. The public facade owns the untyped direct Publish command.
5. The processor owns Performance activation eligibility/staging.
6. Public published lifecycle truth remains string-only.
7. Mutable facade macro values are not bound to an immutable published schema.

The initial direct execution must report exactly seven seams and return exit 1. An unexpected exit 0,
exit 2, or different count blocks Mini Sprint 6.1 because it means the baseline is stale or broken.

## Mini Sprint 6.2 update

The facade now captures typed identity through `PerformancePublishController`, accepts worker results
only through exact controller eligibility, and reconciles Pending/Active with the existing processor
activation path. The facade wrapper remains temporarily callable by shells, but it no longer owns
request/result lifecycle. The direct audit now reports exactly **six** remaining seams.

## Mini Sprint 6.3 update

Full-project immutable preparation now validates authored/prepared/route/source/macro digests and
rejects partial or mixed-revision results before activation staging. This slice intentionally does
not own any of the six remaining shell, processor-activation, presentation, or macro-value seams, so
the direct audit remains stable at exactly **six**.

## Mini Sprint 6.4 update

Bounded cross-lane scheduling now enforces newest-only candidates, Publish priority with Preview
fairness, cooperative in-flight cancellation, completion backpressure, and explicit time/depth/memory
budgets. This slice owns none of the six remaining shell, processor-activation, presentation, or
macro-value seams, so the direct audit remains stable at exactly **six**.

## Mini Sprint 6.5 update

The processor no longer owns a `stagePerformanceActivation()` eligibility branch. One immutable
token/build/digest authorization now comes from the Publish controller before renderer staging and
must match the audio-boundary acknowledgement. Staging rejection preserves exact last-known-good,
and old payloads retire through bounded off-audio tokens. The direct audit now reports exactly
**five** remaining seams.

## Mini Sprint 6.6 update

Generation-owned held and releasing voices do not change the five remaining shell, presentation,
or macro-binding seams. The direct audit therefore remains stable at exactly **five**.

## Mini Sprint 6.7 update

The Performance authorization payload now carries an immutable, revision/digest-matched published
macro table, and the processor exchanges its bounded callback view with the audio generation at the
same block boundary. Compatible stable IDs preserve and clamp values; new, retired, reordered,
renamed, unassigned, and over-capacity controls have deterministic outcomes. The direct audit now
reports exactly **four** remaining seams, all owned by Mini Sprint 6.8.

## Mini Sprint 6.8 update

All shell and `StatusPanel` Publish actions now submit one typed command through the processor-owned
adapter. The facade publishes one immutable typed presentation snapshot covering Draft, Preview,
requested, active, failed, and last-known-good identities, progress, findings, guidance, and
responsiveness metrics. The direct audit now returns exit 0 with **zero** remaining replacement
seams. Mini Sprint 6.9 owns renaming/registering this executable as a permanent green regression.

## Retirement ownership

| Seam | Owning mini sprint |
|---|---|
| No remaining seams | Retire/register in 6.9 |

## Mini Sprint 6.9 closure

The expected-red source was retired and replaced by the permanent registered green seam audit. The
closure audit also rejects the former `publishedRevisionState` compatibility string and duplicate
facade macro publication state while requiring controller-owned activation payload, typed adapter,
typed presentation, and bounded callback-view seams. It passes with zero remaining replacement
seams.
