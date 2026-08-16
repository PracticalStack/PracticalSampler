# Practical Sampler Phase 4 — Active Documentation and Collateral

Completed August 15, 2026.

## Outcome

Gate G4 passes. Active first-party documentation and maintained collateral now present the product
as **Practical Sampler** and the vendor/company as **Practical Sampler Project**. The edit remained
within the presentation layer: compact component names, source paths, targets, bundle identifiers,
plug-in codes and CIDs, native file extensions, schemas, and package signatures were not renamed.

## Updated source surfaces

- Root and product READMEs, including the application, engine adapter, and HISE content guidance.
- Active architecture, operator, host-validation, package, and contributor documentation.
- Current engineering and UI plans that describe ongoing or future work.
- Product-page titles, metadata, visible copy, image alt text, and accessibility labels.
- The maintained DRSWeb title, description, overview heading, and overview copy.
- The human-readable vendor-provenance headings for the JUCE and HISE dependencies.

The ledger identified 107 approved source occurrences across 59 files. Those occurrences were
updated at their source; generated reference copies were not edited independently.

## Reference mirror

`npm run sync:references` regenerated `DRSWeb/protected-content/reference` from the workspace root,
the product README, and the product `docs` tree. A SHA-256 source-to-destination comparison covered
235 source/mirror pairs with zero missing files and zero mismatches after the final synchronization.

Because the established synchronization task refreshes the complete reference library, its output
also brings previously unsynchronized maintained documents into the protected mirror. This is
generated source-to-mirror catch-up, not a second authored copy of the Phase 4 edits.

## Preserved records and explicit exceptions

- Dated build and host evidence retains the artifact names and host results that were true when the
  evidence was produced. Mirrored evidence has the same historical classification as its source.
- The identity initiative, the superseded broad rename proposal, and phase evidence retain both
  names because they explain and control the migration.
- External REAPER projects remain untouched under the initiative decision not to provide old-session
  compatibility.
- The installer retains the old VST3 bundle name only as an explicit stale-artifact removal target.
- Twelve partial `Decent Rhapsody` strings remain deferred by the exact-name scope; they are not
  company/vendor fields and were not part of the approved `Decent Rhapsody Studio` replacement.

No historical record was rewritten to look as though it was produced by Practical Sampler.

## Validation

| Check | Result |
| --- | --- |
| Presentation identity audit | Passed; 213 remaining old-name occurrences are all classified, with zero `CHANGE` and zero unclassified occurrences |
| Remaining classifications | 164 `DEFER`, 43 `PRESERVE-HISTORICAL`, 6 `PRESERVE-TECHNICAL` |
| Source-to-mirror SHA-256 comparison | Passed; 235 pairs, zero missing, zero mismatches |
| DRSWeb ESLint | Passed |
| DRSWeb optimized production build | Passed; Next.js compiled, type-checked, and generated all six routes |
| Technical identity baseline | Passed; project/target names, Dcrh/Drs0 codes, both VST3 CIDs, four native formats, and 16 host parameter IDs remain stable |

Phase 3's unrelated clean full-suite failures remain recorded separately and do not invalidate this
editorial gate.
