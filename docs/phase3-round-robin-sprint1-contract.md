# Phase 3 Round Robin Sprint 1 - Contract And Fixture Baseline

Frozen July 25, 2026.

This note is the Sprint 1 execution artifact for
`phase3-1-3-round-robins-development-roadmap.html`. It takes the roadmap out of the planning state
and freezes the first-release Round Robin rules that later schema, runtime, UI, and import slices
must implement consistently.

## Review findings resolved in Sprint 1

The roadmap was directionally strong, but Sprint 1 needed to stop a few important details from
remaining implicit:

- Round Robin selection is frozen as `sequential` only for the first completed release. Random,
  weighted, and no-repeat policies remain out of scope until the native model can represent them
  honestly.
- Round Robin slot numbers are frozen as `1`-based, not `0`-based. A slot is considered active
  only when both slot count and slot index are positive and the index lies inside the count.
- Round Robin selection is frozen as a pool-level decision. If one note-on needs multiple sibling
  routes at once because of velocity crossfade, every participating route must come from the same
  Round Robin slot.
- The current repository still uses scalar `roundRobinLength` and `roundRobinPosition` values and a
  voice-id modulo selector. Sprint 1 records that as the baseline implementation seam, not as the
  final feature contract.

These decisions remove the largest remaining source of disagreement that could otherwise leak into
schema work or runtime routing later in Phase 3.1.3.

## First-release supported shape

Sprint 1 freezes a deliberately narrow first-pass Round Robin contract:

- Sequential stepping only
- Slot counts and slot indexes are positive integers
- Slot indexes are `1..slotCount`
- A non-RR zone remains a plain always-eligible zone
- A valid sequential RR family may expose one route per slot or a same-slot sibling set per slot
- Crossfade siblings must agree on RR slot identity when they represent the same note trigger
- Imported SFZ `seq_length` and `seq_position` are the canonical external reference shape for the
  first release

This matches the checked-in pseudo-RR Rhodes corpus and is intentionally smaller than the full SFZ
sequencing surface.

## Frozen runtime and pairing semantics

Sprint 1 freezes these behavior rules even though later sprints still need to improve the internal
representation:

- The repository's current runtime selector is a simple `voiceId % roundRobinLength` cycle over the
  matching trigger family.
- Selection remains deterministic and `1`-based.
- A route with RR metadata participates only when its slot equals the selected slot.
- Crossfade topology pairing already treats RR slot identity as part of the pairing contract:
  same-note crossfade siblings must match on both RR length and RR position.
- A same-note overlap across different RR slots is not a valid crossfade pair.

The last two rules are already enforced by `VelocityCrossfade.h` topology pairing and must remain
true after explicit pool identity lands.

## Native filename heuristic baseline

Sprint 1 also freezes the current native filename-import heuristics as the baseline seam:

- `rr2`, `take3`, and `roundrobin4` are recognized as Round Robin slot tokens
- the current heuristic surface only preserves a flat `roundRobinIndex`
- it does not yet infer pool identity, slot count, or sibling completeness from neighboring files

That is acceptable for Sprint 1 because later import work is expected to replace this flat
representation with explicit pool modeling.

## Fixture inventory

The Sprint 1 fixture set is now explicit:

- Generated pseudo-RR corpus root:
  - `DemoSFVInstruments/jlearman.jRhodes3d-master-rr`
- Primary mono RR + crossfade fixture:
  - `DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz`
- Primary mono RR no-crossfade control:
  - `DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-no-xfade-flac.sfz`
- Stereo RR + crossfade fixture:
  - `DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-st/_jRhodes3d-st-flac.sfz`
- Stereo RR no-crossfade control:
  - `DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-st/_jRhodes3d-st-no-xfade-flac.sfz`
- Stereo vibrato RR + crossfade fixture:
  - `DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-sv/_jRhodes3d-sv-flac.sfz`

Sprint 1 freezes the surrounding corpus shape:

- `3` generated RR branches: mono, stereo, stereo vibrato
- `195` FLAC samples per branch
- `585` FLAC samples total
- `12` rewritten SFZ files total
- `225` regions in each direct-load branch fixture
- `75` regions in each slot position for the 3-way RR fixtures

If that checked-in pseudo-RR corpus changes, the characterization test must change in the same
review.

## Open seams in Sprint 1

Sprint 1 intentionally leaves these seams open:

- no explicit authored Round Robin pool identity field in the native schemas
- no RR playback mode field in the native schemas
- no pool-scoped runtime counters in preview or publish playback lanes
- no authoring UI for inspecting, grouping, or repairing RR pools
- no import review language that distinguishes supported sequential RR from unsupported sequencing
  policies

Those gaps are documented in:

- `docs/phase3-round-robin-sprint1-red-tests.md`

and encoded as the direct-only expected-red audit:

- `drs_phase3_round_robin_contract_red_tests`

## Test coverage frozen by this slice

Green contract coverage:

- `drs.phase3.round_robin_contract`
- `drs.phase3.round_robin_fixture_profile`

Direct-only expected-red coverage:

- `drs_phase3_round_robin_contract_red_tests`

## Non-goals of Sprint 1

Sprint 1 does not yet:

- add explicit Round Robin pool objects to project or instrument schemas
- replace modulo-based runtime selection with pool-scoped counters
- add Round Robin editing UI
- add import-side pool repair or completeness inference
- reclassify any new RR import behavior as fully converted beyond standard sequential SFZ semantics

Those remain intentionally gated by later sprints in the roadmap.
