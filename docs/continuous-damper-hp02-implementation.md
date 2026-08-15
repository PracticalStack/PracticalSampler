# HP-02 — Native damper metadata and Salamander projection

Status: complete on August 15, 2026.

HP-02 adds persistence and import support only. It does not change live voice,
release-envelope, or repedaling behavior; those seams remain owned by HP-03 and
HP-04. Pedal noise, sympathetic resonance, and physical string coupling remain out
of scope.

## Persisted contract

- Project schema 7 / authoring schema 6 stores a `damper` record on every zone.
- Runtime instrument schema 5 carries the same record through compilation and
  package export.
- The record contains the sustain controller and threshold, dynamic-release flag,
  CC64 release amount, authored curve index, and an immutable 128-value compiled
  curve.
- Schema 6/5 projects migrate to 7/6 with the HP-01 compatibility defaults: CC64,
  threshold 64, and dynamic release disabled.
- Plugin and standalone project-open paths include this final migration step.
- Project, snapshot, prepared-playback, and runtime-instrument serializations include
  damper metadata in deterministic digest/cache inputs.
- The existing `.drpkg` container and authentication format are unchanged.

## Curve policy

Sparse `v000` through `v127` points are sorted and linearly interpolated. Values
before the first explicit point use the first point's value; values after the last
explicit point use the last point's value. Duplicate indices, malformed or
out-of-range points, non-finite values, invalid references, and unsupported release
controllers produce stable `damper.*` findings.

## Focused SFZ conversion

The importer projects inherited `sustain_cc`, `sustain_lo`, and the complete
Salamander release block:

- `ampeg_dynamic=1`
- `ampeg_releasecc64`
- `ampeg_release_curvecc64`
- the referenced `<curve>` section's `curve_index` and `vNNN` points

When `sustain_lo` is absent from imported focused content, the ARIA-compatible 0.5
default is persisted. Native legacy migrations retain threshold 64.

`ampeg_dynamic` used without the complete CC64 release block is not reinterpreted as
half-pedal metadata. It remains report-only, as do unrelated controller modulation,
resonance, and effect declarations. Only curves referenced by a complete supported
block are converted; other curve declarations remain visible in the report.

## Evidence

The registered `drs.continuous_damper.hp02` test covers:

- sparse-curve compilation, interpolation, endpoint extension, and stable errors;
- the focused Salamander fixture and inherited opcode classification;
- atomic schema migration and deterministic project round trip;
- immutable snapshot and prepared-playback propagation plus digest invalidation;
- runtime instrument schema-5 deterministic round trip;
- unchanged package write, authentication, reopen, and damper preservation; and
- atomic rejection of a missing referenced curve.

Compatibility coverage also passes for the HP-01 contract, binary pedal behavior,
SFZ report and projection suites, prepared playback, packaged FX routing, and package
export lifecycle. `salamander-half-pedal-projection` remains addressable in the
HP-01 direct harness and now exits 0. HP-03 and HP-04 subsequently promoted all six
runtime behavior seams.
