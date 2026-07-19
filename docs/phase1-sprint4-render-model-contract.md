# Sprint 4.1 Immutable Render Model Contract

Status: authoritative Mini Sprint 4.1 boundary, completed July 19, 2026.

## Construction ownership

`buildSamplerRenderModel()` runs only on a message or preparation-owned path. It may allocate the
model and actionable findings. It must never be called by `processBlock()`, note start, voice render,
or voice cleanup. Audio receives only a previously successful `shared_ptr<const SamplerRenderModel>`.

The factory is the only model construction seam. `SamplerRenderModel` has no public constructor and
offers const accessors. A successful model retains its complete const activation payload, preserving
snapshot, prepared handles, decoded PCM, revision identity, and eventual non-audio retirement.

## Renderer-facing contents

The model contains:

- lane, revision, snapshot build ID, prepared build ID, and both content digests;
- a stable prepared-sample table with source/stream identity, sample rate, frame/channel counts, and
  shared immutable decoded PCM;
- a stable route table with zone/source identity, sample index, MIDI key/velocity ranges, root key,
  gain, pan, start frame, and loop range;
- the retained activation payload ownership lease.

It intentionally excludes source paths, path resolution policy, document objects, worker state,
diagnostic strings, device state, mutable voices, and shell/UI objects.

## Required prepublication validation

Construction fails when any of these conditions is present:

- missing, ineligible, or lane/lifecycle-incoherent activation payload;
- missing snapshot/prepared handles, zero build identities, revision mismatch, or digest mismatch;
- empty sample/route topology or unequal snapshot/prepared route counts;
- empty or duplicate zone identity;
- missing decoded PCM, invalid/unsupported channel count, invalid sample rate/frame count, channel
  metadata mismatch, or PCM shorter than the declared frame count;
- out-of-range sample index or mismatched zone/sample/stream identity;
- invalid root key, key range, velocity range, gain, pan, or start frame;
- enabled loop outside the retained half-open frame range `[loopStartFrame, loopEndFrame)`;
- a prepared route missing from the snapshot or differing from snapshot topology.

Failures return stable `render-model-*` finding codes and never return a partial model.

## Callback vocabulary

- `SamplerAudioBufferView` is a non-owning writable channel-pointer view.
- `SamplerRenderEventView` is a non-owning read-only event view; an empty view may have null storage.
- `SamplerRenderRequest` combines output, events, and output sample rate without owning containers.
- `SamplerRenderResult` publishes primitive frame/event/voice counters without strings or vectors.

Mini Sprint 4.1 defines these types but does not implement callback DSP. Event range validation,
sample-accurate scheduling, voice allocation, and overflow policy belong to 4.2 and 4.3.

## Supported topology

The initial renderer boundary accepts mono or stereo decoded float PCM with finite positive sample
rate, non-zero frame count, and one stored channel vector per declared channel. Compiled stream
topology is optional because general authored preparation can supply decoded-memory handles.

## Build and test integration

- Product target: `drs_sampler_core`
- Focused executable: `drs_sprint4_render_model_tests`
- CTest name: `drs.sprint4.render_model`
- Aggregate: `drs_all_tests`

See [the renderer boundary audit](phase1-sprint4-renderer-boundary-audit.md) for the migration map and
[the Sprint 4 entry-gate contract](phase1-sprint4-entry-gate-contract.md) for upstream assumptions.
