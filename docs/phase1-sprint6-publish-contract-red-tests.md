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

## Retirement ownership

| Seam | Owning mini sprint |
|---|---|
| Facade command/controller boundary | 6.2 |
| Processor activation eligibility/staging | 6.5 |
| Immutable published macro binding | 6.7 |
| Three direct shell/UI calls and typed presentation | 6.8 |

Mini Sprint 6.9 removes any remaining compatibility branches and converts the executable into a
registered green `drs.sprint6.publish_contract_seams` regression target.
