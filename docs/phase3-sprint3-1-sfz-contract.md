# Sprint 3.1.1 SFZ Contract And First Fixture Baseline

Frozen July 21, 2026. This is the authoritative contract for the first Phase 3 SFZ import slice.
Later slices may replace temporary seams and add real parser, report, and UI code, but they must
not change these decisions without an explicit contract revision and updated tests.

## Outcome

SFZ import is an analyze-first workflow. The product must parse and classify an SFZ document before
it is allowed to mutate the current authored project. Final conversion is a separate explicit
creator action that may occur only after the review surface presents unsupported, lossy, and
blocking findings.

Sprint 3.1.1 does not ship the parser yet. It freezes the typed workflow semantics, the review
gate rules, and the first checked-in fixture profile so later implementation slices can be measured
against one unambiguous baseline.

## Typed command and review semantics

Two command intents are frozen:

- `SfzImportCommandType::analyzeDocument`
- `SfzImportCommandType::commitReviewedImport`

`analyzeDocument` authorizes discovery, parsing, normalization, validation, classification, and
projection into a provisional native result. It does not authorize project mutation.

`commitReviewedImport` authorizes final application of a previously reviewed import result. It must
never be legal without a completed review step.

## Typed lifecycle

The ordinary document flow is:

- `idle -> discovering -> parsing -> normalizing -> validating -> classifying -> projected -> reviewReady -> committed`

Blocked or canceled outcomes are explicit, not implicit:

- discovery, parsing, normalization, validation, classification, and projection may become
  `blocked` or `canceled`
- `reviewReady` may restart analysis or cancel instead of committing
- `blocked`, `canceled`, and `committed` may restart from `idle` or `discovering`

Import may not skip directly from `idle` to `reviewReady`, from `parsing` to `committed`, or from
`blocked` to `committed`.

## Finding and review policy

Every recognized SFZ construct must land in exactly one disposition:

- `converted`
- `approximated`
- `reportedOnly`
- `blocking`

Review policy is derived from findings:

- converted informational findings require no extra confirmation
- approximated or reported-only findings require explicit confirmation before commit
- blocking findings prevent commit entirely

This is the core Phase 3 product rule: unsupported or lossy behavior may be imported only when it
is surfaced first.

## First fixture baseline

The first implementation target remains:

- `DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz`

Sprint 3.1.1 freezes the following characterization for that fixture:

- 1 `control` header
- 1 `master` header
- 5 `group` headers
- 225 `region` headers
- 1 `curve` header
- 3-way round robin via `seq_length=3` and balanced `seq_position=1/2/3`
- 5 velocity layers
- 4 group-level velocity-crossfade declarations
- local relative `.flac` sample references on every region
- CC1 label/default/width declarations plus curve index 99

If that checked-in fixture changes, the characterization test must change in the same review.

## Open seams in Sprint 3.1.1

Sprint 3.1.1 intentionally leaves these seams open:

- no product-owned SFZ parser API
- no parser implementation
- no shared review-model surface for the shells
- no standalone `.sfz` chooser or review path
- no plug-in `.sfz` chooser or review path
- no engine-facade SFZ import entry point

Those gaps are documented in:

- `docs/phase3-sprint3-1-sfz-contract-red-tests.md`

and encoded as the direct-only expected-red audit:

- `drs_sprint31_sfz_contract_red_tests`

## Test coverage frozen by this slice

Green contract coverage:

- `drs.sprint31.sfz_contract`
- `drs.sprint31.sfz_fixture_profile`

Direct-only expected-red coverage:

- `drs_sprint31_sfz_contract_red_tests`

## Non-goals

Sprint 3.1.1 does not implement:

- the real SFZ parser
- include resolution
- opcode normalization
- compatibility reporting UI
- native project projection
- final project mutation from an SFZ document

Those begin in Sprint 3.1.2 and later.
