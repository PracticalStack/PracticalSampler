# Phase 3 Crossfade Sprint 1 - Contract And Fixture Baseline

Frozen July 21, 2026.

This note is the Sprint 1 execution artifact for `phase3-1-2-crossfade-support-development-roadmap.html`.
It takes the roadmap out of the planning state and freezes the first-release rules that later
model, compile, runtime, and review slices must implement consistently.

## Review findings resolved in Sprint 1

The roadmap was directionally strong, but two details needed to stop being implicit before code
spread across the stack:

- The relationship between crossfade metadata and the existing `velocityLow` / `velocityHigh`
  fields was not explicit. Sprint 1 resolves that by treating `velocityLow` and `velocityHigh` as
  the non-zero audible window for a zone. Crossfade metadata stores absolute fade endpoints inside
  that window.
- Boundary ownership at zero-gain endpoints was not explicit. Sprint 1 resolves that by defining
  participation in terms of `gain > 0.0`. The outgoing layer owns the fade-start velocity, the
  incoming layer owns the fade-end velocity, and only interior velocities start two voices.

These two decisions remove the main source of disagreement that could have leaked into Sprint 2
serialization or Sprint 4 runtime behavior.

## First-release supported shape

Sprint 1 freezes a deliberately narrow first-pass crossfade contract:

- Velocity crossfade is linear only.
- A zone may have no fade, one fade-in, one fade-out, or both a fade-in and fade-out.
- Fade bounds are stored as absolute velocities, not lengths.
- Fade-in is anchored to the zone's `velocityLow`.
- Fade-out is anchored to the zone's `velocityHigh`.
- A middle zone may carry both fades only when `fadeInHigh < fadeOutLow`.
- Supported neighboring layers must mirror the same overlap interval:
  - lower layer `fadeOutLow/high == upper layer fadeInLow/high`
- A supported note-on may resolve to one or two adjacent layers, never more.
- The runtime must not start a zero-gain voice just because the raw velocity lands on a fade
  boundary.

This matches the checked-in Rhodes corpus shape and is intentionally smaller than the full SFZ
crossfade surface.

## Gain policy

The frozen gain rule for the first release is linear and deterministic:

- Outside a zone's audible window, gain is `0.0`.
- With no fade present, in-range gain is `1.0`.
- Fade-in ramps from `0.0` at `fadeInLow` to `1.0` at `fadeInHigh`.
- Fade-out ramps from `1.0` at `fadeOutLow` to `0.0` at `fadeOutHigh`.
- When both adjacent layers participate inside a mirrored overlap, their gains should sum to `1.0`
  within floating-point tolerance.
- Endpoint ownership is asymmetric by design:
  - at `fadeInLow`, only the lower layer is active
  - at `fadeInHigh`, only the upper layer is active

The helper implementation for these rules lives in
`engine_adapter/include/drs/engine/VelocityCrossfade.h`.

## Native representation choice

Sprint 1 freezes the internal representation choice requested by the roadmap:

- Store absolute crossfade endpoints only.
- Keep `velocityLow` / `velocityHigh` as the effective non-zero audible window.
- Do not store derived fade lengths or inferred neighbor IDs in authored state.

That keeps Sprint 2 persistence simple and leaves Sprint 3 free to derive adjacency tables at
compile time instead of serializing neighbor wiring directly.

## Fixture inventory

The first Sprint 1 fixture set is now explicit:

- Primary mono crossfade fixture:
  - `DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz`
- Stereo crossfade fixture:
  - `DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-st.sfz`
- No-crossfade control:
  - `DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono-no-xfade.sfz`

Sprint 3.1.6 already broadened the surrounding corpus. Sprint 1 only needs one crossfade control
set to freeze the contract cleanly before the larger hardening matrix is revisited.

## Baselines captured before implementation

Current reviewed/import baseline:

- The mono and stereo crossfade fixtures currently produce `16` `sfz.velocity_crossfade.approximated`
  findings and remain `reviewReady` with confirmation required.
- The no-crossfade control fixture currently produces `0` velocity-crossfade approximation findings.
- Those baselines are already guarded by `drs.sprint31.sfz_compatibility` and
  `drs.sprint31.sfz_corpus_hardening`.

Current runtime baseline:

- The sampler currently starts every overlapping matching layer additively when ranges overlap.
- `drs.sprint4.scheduler` already proves the existing overlap behavior is plain additive, not
  crossfade-aware.
- That means Sprint 4 of the roadmap must change gain behavior without regressing the fixed-capacity,
  no-allocation scheduling guarantees already enforced by Sprint 4 tests.

## Coverage map

Sprint 1 now has named homes for the next layers of coverage:

- Contract and boundary math:
  - `drs.phase3.crossfade_contract`
- Fixture shape and inventory:
  - `drs.phase3.crossfade_fixture_profile`
- Existing review/report baseline:
  - `drs.sprint31.sfz_compatibility`
  - `drs.sprint31.sfz_corpus_hardening`
- Existing overlapping-layer runtime baseline:
  - `drs.sprint4.scheduler`

Planned future homes:

- Sprint 2 persistence/model round-trip: extend native project/instrument serializer coverage.
- Sprint 3 compile/prepared propagation: extend compile and prepared-playback tests.
- Sprint 4 runtime mixing: extend sampler scheduler / offline renderer coverage with crossfade gain
  assertions.
- Sprint 5 review promotion: extend SFZ compatibility and corpus-hardening expectations.

## Non-goals of Sprint 1

Sprint 1 does not yet:

- add crossfade fields to authored project or instrument schemas
- change the current importer projection
- change sampler route resolution or voice gain
- promote crossfade findings from `approximated` to `converted`

Those remain intentionally gated by later sprints in the roadmap.
