# Sprint 4.5 Playback Context And Activation Lifetime Contract

Completed July 19, 2026. This contract defines the mutable lane boundary around the shared,
shell-independent sampler renderer.

## Context ownership

Each Preview or Performance lane owns a distinct `SamplerPlaybackContext`. A context contains:

- one inline 24-slot `SamplerVoicePool`, including note ownership;
- one inline 128-event `SamplerEventBlock` scratch buffer;
- primitive render, voice, reset, activation, and retirement counters;
- the current raw, const render-model view used by audio;
- four fixed activation slots that retain immutable render models and their payloads; and
- an eight-token single-producer/single-consumer retirement queue.

The renderer writes directly to the caller's non-owning output view, so no context-owned output
scratch is required. No mutable voice, event, note, counter, or activation-slot storage is shared
between Preview and Performance. The two contexts may execute the same renderer code concurrently.

## Block-boundary activation

`stageActivation()` is message-owned. It rejects null models and models for the other lane, drains
returned retirement tokens, fills a free slot, and publishes only the integer slot index with
release ordering. Superseded pending models are reclaimed on the message owner.

`renderBlock()` exchanges the pending integer at the start of the block with acquire/release
ordering. Only then does it update the raw current-model view and process that block's events. It
does not copy, move, reset, or destroy a `shared_ptr`. Events at the activation boundary therefore
start from the new model, while voices started earlier keep their original const model pointers.

The exchange is bounded to four activation slots. Staging fails deterministically if all slots are
active or retained by old voices; it never grows capacity on the callback.

## Old-model voice leases and retirement

An active or releasing `SamplerVoice` is the primitive lease on the activation slot whose model it
references. Replaced activation slots remain in an inline retired list while any such voice uses
their model. Natural completion, release completion, stealing, and reset clear the voice's raw
model/route/sample views.

Once a retired model has no live voices, audio enqueues only `{slotIndex, serial}`. The serial
prevents a stale token from reclaiming a reused slot. `serviceRetirements()` is message-owned and is
the only returned-token path that resets the slot's shared model and performs final payload/PCM
release. Context construction, destruction, and final retirement drain are likewise message/control
owner responsibilities.

## Reset, restart, and close

- `resetAtBlockBoundary()` stops only that context's voices, clears its event scratch, preserves its
  current activation, and makes newly unleased retired slots eligible for return.
- `prepare(newSampleRate)` is the device-restart boundary: it stops that context's voices, updates
  the output rate, and preserves/rebinds its active immutable model.
- `closeAtBlockBoundary()` stops voices, detaches active and pending activation indices, and returns
  them as retirement tokens. Final release still waits for `serviceRetirements()`.

Reset, restart, close, note events, and retirement in one context cannot inspect or mutate the other
context. Counters remain lane-local and cumulative so diagnostics do not become a shared mutable
renderer dependency.

## Real-time limits

Audio-owned context operations are fixed-array scans and primitive atomic/index operations. They do
not allocate, grow a container, resolve paths, decode samples, copy shared ownership, or destroy a
model/payload. Model construction, activation staging, superseded-pending cleanup, returned-token
drain, and final context destruction stay off audio.
