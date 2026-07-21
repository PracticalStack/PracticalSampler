# Sprint 3.1.3 SFZ Compatibility Report Baseline

Recorded Tuesday, July 21, 2026.

Sprint 3.1.3 promotes the SFZ importer from "parsed and normalized" to "reportable and reviewable"
without yet mutating the authored project.

## Outcome

The product now has:

- a product-owned compatibility report seam in `engine_adapter/include/drs/engine/SfzImportReport.h`
- a first classifier implementation in `engine_adapter/src/SfzImportReport.cpp`
- an explicit engine-facade analysis entry point for later shell integration
- a shared shell-facing report model in `app/src/shared/SfzImportReportModel.h`

The importer still does not finalize native project mutation in this slice. It now produces the
typed report payload that later projection and UI work will consume.

## First-fixture support profile frozen by this slice

For `DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz`,
the current compatibility report freezes these counts:

- 1608 local opcodes traced
- 1583 currently classified as `converted`
- 16 currently classified as `approximated`
- 9 currently classified as `reportedOnly`
- 0 currently classified as `blocking`
- 25 warning findings and 0 error findings

The first fixture therefore lands in `reviewReady` with
`SfzImportReviewDisposition::confirmationRequired`.

## Intentional support decisions in Sprint 3.1.3

- `lovel`, `hivel`, `lokey`, `hikey`, `pitch_keycenter`, `sample`, `seq_length`, `seq_position`,
  `volume`, and `ampeg_release` are treated as currently convertible.
- `xfin_*` and `xfout_*` are treated as lossy approximations and must be surfaced before final
  import.
- `label_cc1`, `set_hdcc1`, `width_oncc1`, `width_curvecc1`, and `<curve>` data are preserved as
  report-first transparency items.
- any otherwise recognized but still unmapped opcode falls back to a stable
  `sfz.opcode.unmapped` report entry instead of being silently dropped.

## Delivered tests

Registered green tests:

- `drs.sprint31.sfz_contract`
- `drs.sprint31.sfz_fixture_profile`
- `drs.sprint31.sfz_parser`
- `drs.sprint31.sfz_normalization`
- `drs.sprint31.sfz_compatibility`
- `drs.sprint31.sfz_report_model`

Direct-only expected-red audit:

- `drs_sprint31_sfz_contract_red_tests`

## Red seams retired by Sprint 3.1.3

The direct audit no longer treats these as open gaps:

- missing shared `app/src/shared/SfzImportReportModel.h`
- missing engine-facade SFZ document analysis entry point

## Open seams after Sprint 3.1.3

These remain intentionally open for later slices:

- standalone-shell `.sfz` chooser and review path
- plug-in-shell `.sfz` chooser and review path
- projection from the normalized SFZ/report model into provisional native project entities
- explicit final review-confirm/commit workflow in the shells
