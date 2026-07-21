# Sprint 3.1.2 SFZ Parser And Normalization Baseline

Recorded Tuesday, July 21, 2026.

Sprint 3.1.2 lands the first real product-owned SFZ document seam:

- `engine_adapter/include/drs/engine/SfzImport.h`
- `engine_adapter/src/SfzImport.cpp`

## Outcome

The product can now:

- load an SFZ document from disk
- follow textual `#include` expansion
- preserve source-file, line, column, scope, and opcode provenance
- parse headers and opcode assignments into product-owned sections
- normalize control/global/master/group/region inheritance into effective opcode views

This is still not the compatibility-report or UI slice. It is the parser and normalization seam that
later compatibility classification and project projection will consume.

## Delivered tests

Registered green tests:

- `drs.sprint31.sfz_contract`
- `drs.sprint31.sfz_fixture_profile`
- `drs.sprint31.sfz_parser`
- `drs.sprint31.sfz_normalization`

Direct-only expected-red audit:

- `drs_sprint31_sfz_contract_red_tests`

## Red seams retired by Sprint 3.1.2

The red audit no longer treats these as open gaps:

- missing product-owned SFZ parser API header
- missing product-owned SFZ parser implementation

## Open seams after Sprint 3.1.2

These remain intentionally open for later slices:

- shared SFZ review-model surface
- standalone-shell `.sfz` chooser and review path
- plug-in-shell `.sfz` chooser and review path
- engine-facade SFZ import entry point
- compatibility engine and user-facing report model
- projection from normalized SFZ content into native project entities

## Normalization rule frozen by this slice

This slice freezes the current inheritance model used by the tests:

- `control` and `global` accumulate and persist
- a new `master` replaces the previous master scope
- a new `group` replaces the previous group scope
- `region` effective opcodes inherit `control`, `global`, current `master`, current `group`, then local overrides

If later SFZ compatibility work needs to change this behavior, it must update this document and the
normalization tests together.
