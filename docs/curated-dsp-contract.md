# Curated DSP Contract

Status: Accepted  
Decision date: July 30, 2026  
Scope: Curated DSP schema 5, authored representation 4, and the product-owned render path  
Task: DSP-00-01

## Purpose

This ADR freezes the rules that govern the curated DSP feature before model, routing, or
audio-path changes are made. It is the implementation contract for the companion
`curated-dsp-system-design.html` and `curated-dsp-development-plan.html` documents.

## Scope and ownership

1. Each zone, group, and instrument master owns at most one ordered serial insert chain.
   A slot belongs to exactly one chain. Chains do not share slots.
2. Signal order is zone voices summed into the zone chain, zone output accumulated into the
   group chain, and group output accumulated into the instrument chain. Existing route gain
   and pan are retained at their current points in this path.
3. Aux sends, bus-to-bus graphs, feedback routes, sidechains, third-party plug-in hosting,
   convolution assets, oversampling, and host latency compensation are out of scope.
4. The public data and graph contracts are product-owned. JUCE primitives remain
   private implementation details and do not appear in persisted models or snapshot APIs.

## Authored model and compatibility

1. Schema 5 / authoring schema 4 add a versioned effect type and ordered stable
   parameter-ID/value records to every effect slot. Labels are presentation only.
2. A catalog descriptor owns the type ID, algorithm version, legal scopes, parameter units,
   ranges, defaults, smoothing, state/tail class, memory, and cost metadata.
3. Unknown effect types, versions, and parameter records are retained exactly where possible,
   reported with stable findings, and bypassed. They never produce a partial executable graph.
4. Schema-4 FX metadata migrates with original IDs and order preserved. Every migrated slot is
   runtime-bypassed and marked `review to enable`, even if legacy metadata was not bypassed.
   A project must therefore render identically before an explicit authored enable action.

## Graph, activation, and real-time rules

1. A snapshot captures all DSP topology and values needed to compile a graph without reading
   mutable project state. Canonical owner source, slot order, type, version, bypass, and each
   parameter value contribute to the authored graph digest; labels do not.
2. Graph compilation is off-audio and produces a flat, immutable zone -> group -> master plan.
   It resolves destination indices, fixed parameter slots, scratch slices, memory requests, and
   bounded costs with overflow-safe accounting. Invalid or over-budget graphs are rejected
   before activation with stable paths.
3. A render generation owns the immutable sampler model and plan plus exclusive mutable effect
   histories and preallocated scratch. Construction, preparation, destruction, and resource
   release occur off the audio callback.
4. At a block boundary the callback exchanges only primitive activation/control tokens. It does
   not allocate, lock, touch the filesystem, parse JSON, construct strings, release ownership,
   or destroy objects.
5. Preview and Performance use independently prepared generations and never share mutable DSP
   state. Diagnostics are primitive snapshots; UI code never inspects an effect instance.
6. The zero-node graph selects the existing direct path. A bypassed one-node chain has a
   pass-through fast path. Those paths must preserve the frozen dry baseline.

## Parameters, bypass, and macros

1. Topology changes use an authored transaction and activation. Value changes use a bounded,
   generation-tagged parameter publication path and do not rebuild topology.
2. Catalog-declared linear or logarithmic smoothing is effect-owned, resettable, and stable
   across supported block partitions and sample-rate changes.
3. Slot and chain bypass use a short equal-power dry/processed crossfade. Expensive processing
   may suspend only after the declared tail has drained.
4. The existing fixed host macro topology remains the only DAW-visible DSP control surface.
   A macro targets a stable slot ID and parameter ID with explicit source/destination range and
   curve metadata; it is resolved to bounded control slots during publish preparation.

## Reset, transport, tails, and latency

1. Curated version-1 effects add no reported host latency. Effects needing latency compensation
   require a later, explicitly versioned design.
2. `reset` and panic immediately clear voices and effect state. Normal note-off and
   all-notes-off allow bounded tails. Stop, seek, loop jump, and device/sample-rate change follow
   effect-specific documented reset/discontinuity rules.
3. Stateful effects declare a maximum tail class. Feedback is clamped below unity; a silence
   detector may retire early, but a hard frame/time ceiling guarantees retirement. Under
   generation pressure the oldest retained generation fades out with a diagnostic counter.
4. Transport is a sanitized primitive input (tempo, time signature, play state, sample position,
   validity flags). Tempo-aware effects use the frozen fallback of 120 BPM for missing or invalid
   host data. Core DSP never calls a host API.

## Provisional hard budgets

These S0 limits are deliberately conservative and apply before an activation is accepted. S12
may tighten catalog costs or legal ceilings, but may not weaken the safety caps without revising
this ADR and its verification evidence.

| Resource | Provisional limit | Enforcement |
|---|---:|---|
| Active render generations per lane | 4 | Reject or apply explicit oldest-generation pressure fade |
| Retired generation queue per lane | 8 tokens | Diagnostic + bounded pressure policy |
| Effect slots per project | 128 | Structural loader / host-state rejection |
| Parameters per project | 1,024 | Structural loader / graph-plan rejection |
| Graph processing nodes per lane | 128 | Graph-plan rejection |
| Graph scratch memory per lane | 8 MiB | Graph-plan rejection |
| DSP mutable state per generation | 16 MiB | Graph-plan rejection |
| Aggregate retained DSP state per lane | 64 MiB | Activation rejection / pressure policy |
| Per-effect tail ceiling | 30 seconds | Catalog and runtime hard stop |
| Callback processing budget | 50% of the block period | Benchmark diagnostic threshold |
| No-DSP added callback budget | 1% of the block period | Benchmark diagnostic threshold |

## Acceptance audit

This ADR resolves every item in the design document's **Decisions to freeze before
implementation** section: one serial chain per scope, post-sum zone processing, preserved dry
gain/pan behavior, generation-owned mutable state, macro-only host automation, and a
product-owned public contract. It additionally freezes bypass, reset, tails, latency, transport,
legacy migration, and budget enforcement for the development plan.

Any change to these rules requires an ADR revision, updated baseline evidence, and targeted
contract tests.
