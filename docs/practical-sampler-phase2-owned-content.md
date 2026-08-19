# Practical Sampler identity initiative — Phase 2 owned content and generated copy

Date: August 15, 2026  
Status: Complete — Gate G2 passed  
Depends on: Phase 1 / Gate G1

## Outcome

Native product content and newly generated project/package provenance now use `Practical Sampler`
and `Practical Sampler Project`. The former authoring-tool content tree was removed; reusable sample
assets were migrated into the native content root and runtime fixtures remain under product-owned
formats.

## Owned native content

| Surface | New presentation | Preserved technical identity |
|---|---|---|
| `content/samples/DRS_Sine_A3.wav` | Retained as a native source sample fixture. | Stable filename and WAV content. |
| `content/samples/DRS_TriangleLead_A4.wav` | Retained as a native source sample fixture. | Stable filename and WAV content. |
| `content/runtime/phase1/` | Product-owned JSON runtime manifests and negative fixtures. | `.drsproj`, `.drinst`, `.drstrm`, and `.drpkg` schemas and IDs. |

The migrated samples and runtime manifests load through the native content contract. No authoring
tool project metadata, scripts, sample maps, preset backups, or editor scaffolding is retained.

## Generated prose

Newly saved or exported artifacts now receive these free-text notes:

- `.drinst`: `Generated from the current Practical Sampler authoring project when the project was saved.`
- package manifest and packaged `.drinst`: `Exported from the current Practical Sampler authoring project.`

Runtime setup guidance emitted by `EngineFacade` names Practical Sampler. The separate partial
phrase “Decent Rhapsody authoring assets” remains unchanged because it is neither the full old
product name nor a company/vendor field; changing partial product-family copy is outside this
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
| Native content and runtime identity assertions | PASS. |
| Technical identity baseline verifier | PASS. |
| Rolling identity audit | PASS — 533 occurrences, zero unclassified. |

The Phase 0 smoke executable and native content contract test were rerun after the content migration;
both locate the migrated samples and native runtime fixtures.

## Scope audit

Phase 2 removed the authoring-tool content tree and migrated only reusable native sample assets.
Historical identity fixtures remain readable without changing their technical contracts.

No native extension, schema, package signature, bundle ID, plug-in/company code, VST3 CID, host
parameter ID, compact directory, component ID, CMake target, or CTest name changed.

## Gate G2 decision

Gate G2 passes: owned content visibly says Practical Sampler, new save/export provenance uses the
approved identity, legacy free-text notes remain readable without migration, and targeted persistence
and package round-trip tests show no technical contract change.
