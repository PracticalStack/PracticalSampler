# Practical Sampler identity initiative — Phase 2 owned content and generated copy

Date: August 15, 2026  
Status: Complete — Gate G2 passed  
Depends on: Phase 1 / Gate G1

## Outcome

Owned HISE presentation content and newly generated project/package provenance now use
`Practical Sampler` and `Practical Sampler Project`. Existing native artifacts that contain the
former product name only in free-text notes remain readable, and those notes are not migrated or
rewritten.

## Owned HISE content

| Surface | New presentation | Preserved technical identity |
|---|---|---|
| `content/hise_project/project_info.xml` | Name and description say Practical Sampler. | Bundle identifier `com.decentrhapsody.studio` and plug-in code `Drs0`. |
| `content/hise_project/user_info.xml` | Company and copyright say Practical Sampler Project. | Company code `Dcrh`; existing placeholder URL remains unchanged. |
| HISE `Interface.js` | Visible header says Practical Sampler. | Script path, component IDs, and `DRS*` symbols. |
| Owned desktop XML backup | Header text says Practical Sampler. | `DecentRhapsodyStudioUIData` directory, XML filename, component definitions, and geometry. |

Both XML documents and the owned desktop backup parse successfully. The source interface and XML
backup agree on the visible header.

## Generated prose

Newly saved or exported artifacts now receive these free-text notes:

- `.drinst`: `Generated from the current Practical Sampler authoring project when the project was saved.`
- package manifest and packaged `.drinst`: `Exported from the current Practical Sampler authoring project.`

The HISE SDK setup guidance emitted by `EngineFacade` now names Practical Sampler. The separate
partial phrase “Decent Rhapsody authoring assets” remains unchanged because it is neither the full
old product name nor a company/vendor field; changing partial product-family copy is outside this
initiative.

No checked-in deterministic golden artifact contained the changed provenance sentences, so no
golden file was regenerated.

## Compatibility proof

Existing tests were extended with narrow identity-note fixtures rather than introducing a new test
target or renaming an existing one.

`drs.sprint0.import_storage_hardening` proves that:

- a `.drsproj` containing the former product name in a note loads and reserializes with that note
  unchanged;
- a valid `.drinst` containing the former product name in `validationNotes` remains readable;
- a newly generated `.drinst` uses Practical Sampler provenance;
- the project and instrument schema names/versions remain unchanged by these checks.

`drs.performance_package.export_lifecycle` proves that:

- newly exported package-manifest and runtime-instrument notes use Practical Sampler;
- changing only the sealed package manifest's free-text provenance back to the former product name
  still produces a readable package;
- package schema and minimum-reader versions are identical before and after that free-text change.

The two explicit former-name literals are classified by the identity audit's narrow
`legacy-identity-compatibility-fixture` rule as `PRESERVE-HISTORICAL`. They are acceptance fixtures,
not current presentation copy.

## Qualification

| Check | Result |
|---|---|
| Targeted Debug build | PASS — application shared code, engine adapter, storage test, package lifecycle test, and smoke-test executable compile/link. |
| `drs.sprint0.import_storage_hardening` | PASS, 9.33 s. |
| `drs.performance_package.export_lifecycle` | PASS, 2.60 s. |
| `drs.phase3.crossfade_persistence` | PASS, 0.23 s. |
| HISE XML/source identity assertions | PASS. |
| Technical identity baseline verifier | PASS. |
| Rolling identity audit | PASS — 533 occurrences, zero unclassified. |

The Phase 0 smoke executable was built but not run because its old VST3 artifact path and expected
scan name are intentionally scheduled for Phase 3. Phase 2 did not alter those test expectations.

## Scope audit

Phase 2 removed ten active `CHANGE` occurrences and added two intentional compatibility-fixture
occurrences. Remaining former-name occurrences in nearby paths are either the two fixtures, active
documentation reserved for Phase 4, or explicitly deferred partial-name copy.

No native extension, schema, package signature, bundle ID, plug-in/company code, VST3 CID, host
parameter ID, compact directory, component ID, CMake target, or CTest name changed.

## Gate G2 decision

Gate G2 passes: owned content visibly says Practical Sampler, new save/export provenance uses the
approved identity, legacy free-text notes remain readable without migration, and targeted persistence
and package round-trip tests show no technical contract change.
