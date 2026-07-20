# Sprint 6 Publish Expected-Red Seam Audit

Date established: July 19, 2026  
Target: `drs_sprint6_publish_contract_red_tests`  
CTest registration: intentionally absent during Mini Sprint 6.1

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

## Retirement ownership

| Seam | Owning mini sprint |
|---|---|
| Processor activation eligibility/staging | 6.5 |
| Immutable published macro binding | 6.7 |
| Three direct shell/UI calls and typed presentation | 6.8 |

Mini Sprint 6.9 removes any remaining compatibility branches and converts the executable into a
registered green `drs.sprint6.publish_contract_seams` regression target.
