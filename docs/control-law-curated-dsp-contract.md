# Control laws and curated DSP descriptors — Sprint 0 contract

Status: Accepted for implementation  
Decision date: August 2, 2026  
Scope: Control-law initiative Sprints 0–1 (CL-001 through CL-106)

## Decision

Control law is an interaction contract, separate from a DSP parameter's physical
unit and legal range. The ownership boundary is fixed as follows:

| Owner | Responsibility |
|---|---|
| Curated DSP descriptor | Declares the physical unit and valid DSP range; later adds allowed/default/role recommendations. |
| Authoring resolver | Resolves the law and physical range when a target is created or explicitly upgraded. |
| Saved project | Persists the resolved, versioned law and range; it never re-resolves an existing target from a later catalog. |
| Publisher | Validates the authored law and compiles it to bounded, allocation-free runtime data. |
| Perform UI / automation | Operates in normalized coordinates and uses the shared forward/inverse law. |
| DSP | Receives only a finite physical value in its documented unit. It has no presentation or taper policy. |

`CuratedDspParameterDescriptor` remains the one curated-DSP descriptor. A parallel
descriptor is prohibited.

## Stable IDs, versions, and compatibility

The approved initial law IDs are `drs.mixerGain.v1`, `drs.linearDb.v1`,
`drs.bipolarLinear.v1`, `drs.logPositive.v1`, `drs.bipolarCentered.v1`,
`drs.stepped.v1`, and `drs.toggle.v1`. IDs are immutable behavior contracts:
retuning any released law requires a new ID/version, never an in-place change.

Existing targets whose persisted `curve` is `linear` retain the current linear
mapping and exact source/destination range on load, save, publication, recall, and
host automation. They receive a new taper only through the explicit, previewable
"Upgrade mixer taper" migration planned for Sprint 3. Missing, unknown, or future
law versions must fail publication while retaining the last-known-good publication;
they must not silently fall back to a different audible law.

## `drs.mixerGain.v1` anchor contract

The first mixer law is monotonic piecewise-linear interpolation in dB between these
fixed anchors. Values below/above normalized range clamp to the endpoint. Non-finite
normalized or physical inputs are rejected at the authoring/publication boundary and
never reach the callback. At the physical minimum, format as `−∞`; otherwise format
gain in dB.

| Normalized | Physical |
|---:|---:|
| 0.00 | -96 dB |
| 0.05 | -60 dB |
| 0.25 | -30 dB |
| 0.50 | -15 dB |
| 0.75 | -6 dB |
| 0.85 | 0 dB |
| 1.00 | +6 dB |

The inverse law must return the matching normalized anchor and round-trip a finite
physical value within the tolerance introduced by the shared-core tests. Endpoints
are exact.

## Sprint 1 shared-core implementation

`ControlLaw.h/.cpp` compiles an ID plus resolved physical range into a bounded
`CompiledControlLaw` with at most eight anchors. `normalizedToPhysical` and
`physicalToNormalized` are `noexcept`, reject non-finite inputs, clamp finite inputs
at the endpoints, and do not allocate, lock, or inspect UI state. Mixer gain accepts
only its frozen -96…+6 dB range. Positive-log requires a strictly positive range;
bipolar-centered requires a range spanning zero. Formatting is intentionally a
separate UI-facing helper and is never carried in callback data.

## Sprint 2 descriptor policy

Each curated parameter now declares a default law, an allow-list, and optional
role-specific recommendations. Catalog construction rejects unknown laws, duplicate
allow-list entries or roles, defaults outside the allow-list, invalid recommendation
ranges, and any law/range combination the shared core cannot compile. The pure
resolver applies explicit allowed override, then role recommendation, descriptor
default, and finally a unit-based compatibility fallback for incomplete legacy
descriptors.

`drs.gain/gainDb` defaults to `drs.linearDb.v1`, permits
`drs.mixerGain.v1`, and recommends that law with -96…+6 dB for the `mix` role.
Boolean controls use the toggle law; selector-like `character` and `mode` use the
stepped law; positive time/frequency controls use positive-log where their range
permits it; EQ gain uses bipolar-linear. `divisionBeats` is intentionally classified
as continuous positive-log for now: its discrete beat grid and labels are not yet a
catalog contract, so it must not claim a stepped enumeration prematurely.

## Sprint 3 project schema and migration

Structured DSP macro targets may now persist `controlLaw.id` and
`controlLaw.version` beside their resolved destination range. New targets resolve
through the catalog before they are saved; a new `mix`-role Gain target therefore
saves `drs.mixerGain.v1` version 1 and -96…+6 dB. The authoring inspector summary
shows that read-only law/range detail.

An absent control-law object is an intentional legacy representation. Its existing
`curve` and range serialize unchanged and remain the audible compatibility contract.
`AuthoringSession::previewMixerTaperUpgrade()` lists eligible legacy group/bus Gain
targets, while `upgradeMixerTaper()` is the explicit transactional command that
changes them to the mixer law. It marks the project dirty and is fully undoable and
redoable; loading or saving alone never invokes it.

## Current-state parameter inventory

Before Sprint 2 the descriptor contained unit/range/default/smoothing only. The
authoring UI still creates every selected DSP target with role
`mix` and `curve: "linear"`; its editable role vocabulary is `timbre`, `motion`,
`mix`, `placement`, and `other` (plus custom text). A hand-authored `logarithmic`
curve is honored only for positive destination ranges. Thus every entry below is
currently **unclassified**, with linear as the creation default; the proposed role
in the last column is inventory for Sprint 2, not implemented behavior.

| Effect | Parameter | Unit / DSP range | Current treatment | Proposed assignment intent |
|---|---|---|---|---|
| `drs.gain` | `gainDb` | dB, -96…+24 | linear | `mix`: mixer gain; generic: linear dB |
|  | `polarity` | boolean, 0…1 | linear | toggle |
|  | `mute` | boolean, 0…1 | linear | toggle |
| `drs.saturator` | `character` | normalized, 0…2 | linear | stepped/selector |
|  | `driveDb` | dB, 0…36 | linear | modulation |
|  | `tone` | normalized, 0…1 | linear | timbre |
|  | `mix` | normalized, 0…1 | linear | mix |
|  | `outputDb` | dB, -24…+24 | linear | mix / modulation |
| `drs.stereoDelay` | `timeMs` | ms, 1…2000 | linear (log may be authored) | time / log-positive |
|  | `sync` | boolean, 0…1 | linear | toggle |
|  | `divisionBeats` | ratio, 0.0625…4 | linear (log may be authored) | log-positive; stepped grid intentionally unresolved |
|  | `feedback` | ratio, 0…0.95 | linear | modulation |
|  | `pingPong` | boolean, 0…1 | linear | toggle |
|  | `tone` | normalized, 0…1 | linear | timbre |
|  | `width` | normalized, 0…1 | linear | placement |
|  | `mix` | normalized, 0…1 | linear | mix |
| `drs.algorithmicReverb` | `preDelayMs` | ms, 0…250 | linear | time |
|  | `size` | normalized, 0…1 | linear | timbre |
|  | `decaySeconds` | s, 0.1…20 | linear (log may be authored) | time / log-positive |
|  | `damping` | normalized, 0…1 | linear | timbre |
|  | `width` | normalized, 0…1 | linear | placement |
|  | `mix` | normalized, 0…1 | linear | mix |
| `drs.compactEq` | `mode` | normalized, 0…2 | linear | stepped/selector |
|  | `frequencyHz` | Hz, 40…18000 | linear (log may be authored) | timbre / log-positive |
|  | `q` | ratio, 0.25…12 | linear (log may be authored) | timbre / log-positive |
|  | `gainDb` | dB, -18…+18 | linear | `eqGain`: bipolar linear |
|  | `mix` | normalized, 0…1 | linear | mix |
| `drs.chorus` | `rateHz` | Hz, 0.05…5 | linear (log may be authored) | modulation / log-positive |
|  | `depthMs` | ms, 0.1…12 | linear | modulation |
|  | `baseDelayMs` | ms, 5…30 | linear | modulation |
|  | `width` | normalized, 0…1 | linear | placement |
|  | `mix` | normalized, 0…1 | linear | mix |

## Sprint 0 fixture and Sprint 1 core proof

`content/runtime/phase2/control-law/three-group-linear-gain.drsproj` is the
regression fixture for the reported three-group Perform mixer failure. Each exposed
group target uses the legacy 0…1 to -96…+24 dB linear mapping: unity is at 80% travel
and the midpoint is -36 dB. It must remain legacy-linear until an explicit migration.

`drs_control_law_tests` converts the direct-only Sprint 0 seams into registered
deterministic and randomized property tests. It covers anchors, monotonicity,
endpoints, inverse round trips, clamping, non-finite rejection, formatting, and the
legacy fixture. `ControlLaw` is engine-only; its compiled representation is fixed-size
and trivially copyable, while strings are used only by the presentation formatter.
