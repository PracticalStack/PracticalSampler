# Packaged Instrument FX/Routing Release Evidence

Captured 2026-08-14 for delivery slice PX-05.

## Status

Implementation and automated release gates are complete. The dedicated REAPER qualification
matrix is implemented, but its first run timed out before the Lua evidence script executed while
an unrelated interactive REAPER project was active. PX-05 remains pending only that clean host
rerun; no REAPER pass is claimed by this document.

## Compatibility

| Package | Reader | Expected result | Evidence |
| --- | --- | --- | --- |
| Schema v1, graph-free | Schema v2-capable current reader | Load | Checked-in v1 corpus loaded. |
| Schema v2, runtime instrument v4, static FX graph | Current reader | Load and activate graph | Graph and audio parity gates passed. |
| Schema v2, minimum reader v2 | Reader capped at schema v1 | Reject before payload activation | `playbackCompatibilityFailure` returned. |
| Schema v2 with malformed graph | Current reader with an active package | Reject and retain active package | Activation and lifecycle gates passed. |

The v2 metadata dispatch now accepts the caller's supported reader schema version and rejects a
higher `minimumReaderSchemaVersion` before instrument/stream activation. Graph-bearing packages
therefore cannot silently fall back to dry playback in an older reader.

## Audio Parity

The release fixture authors seven slots on five buses. Its active graph compiles to five nodes and
29 parameters, with zone, group, and master scope; serial stereo delay into algorithmic reverb;
slot bypass; chain bypass; mono and stereo sources; and stateful tails.

| Case | Source checksum | Reopened package checksum | Peak | Last non-zero frame | Result |
| --- | --- | --- | ---: | ---: | --- |
| Mono zone/group/master | `9dc76d4b120ed920` | `9dc76d4b120ed920` | 0.0684445 | 56,563 | Exact parity |
| Stereo zone/bypassed-group/master | `114c94f366c08672` | `114c94f366c08672` | 0.0550717 | 50,835 | Exact parity |

Graph digest: `fnv1a64:1ee9c67c40a729d0`  
Plan digest: `fnv1a64:335eb71f33181883`

Repeated package renders are deterministic. The DSP-enabled output differs from the dry oracle,
and both cases retain output beyond frame 36,000 after note-off, proving that the graph is active
and delay/reverb state survives package reconstruction.

## Automated Gates

The following 17 CTest cases pass in `build/eg5-debug`:

- Curated DSP graph plan, gain, stereo delay, reverb, compact EQ, chorus, and scoped routing.
- Sprint 4 deterministic offline renderer.
- Packaged-instrument compile, contract, and activation gates.
- Performance package reader, loader, session, export lifecycle, host validation, and release gate.

Standalone and plugin package activation compile the same graph plan. Editor-closed plugin recall
restores audible package playback and the same plan digest. Reported audio-thread violations are
zero. The Debug VST3 bundle builds successfully.

Machine-readable release report:
`build/eg5-debug/tests/phase1-performance-package-release-gate.json`.

## REAPER Follow-up

Run from the repository root after closing interactive REAPER sessions:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\validation\reaper\run-package-fx-routing-qualification-matrix.ps1
```

The matrix captures a real package-bound VST3 state, injects it into editor-open and editor-closed
projects, and checks 44.1/48 kHz at 128/512 samples for audible finite output and an enabled,
online plug-in. Evidence is written under
`validation/reaper/package-fx-routing-qualification-evidence`.
