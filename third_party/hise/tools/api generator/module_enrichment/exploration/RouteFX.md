# RouteFX - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/RouteFX.h` (lines 44-102), `hi_core/hi_modules/effects/fx/RouteFX.cpp` (lines 70-158)
**Base class:** `MasterEffectProcessor`

## Signal Path

The RouteFX overrides `renderWholeBuffer()` instead of using `applyEffect()` (which is empty). The signal path in `renderWholeBuffer()` (RouteFX.cpp:100-151):

1. Capture pre-routing gain values for metering (if editor is visible)
2. For each source channel: look up the send target channel from the routing matrix
3. If a send target exists: additively copy the source channel to the target channel
4. Capture post-routing gain values for metering

The routing is purely additive - source channels are NOT zeroed after copying. The original signal remains on its source channel while a copy is added to the target channel. This is a send/duplicate operation, not a move.

## Gap Answers

### signal-path: How does the routing matrix process audio?

**Answer:** The module overrides `renderWholeBuffer()` (RouteFX.cpp:100-151) instead of using the standard `applyEffect()` path. `applyEffect()` (line 153-156) is empty.

The routing operates on the full multichannel buffer. For each source channel, `getMatrix().getSendForSourceChannel(i)` returns the target channel index (or -1 for no routing). When a target exists, `FloatVectorOperations::add()` (line 129) additively copies the source to the target. This is a non-destructive add - the source channel retains its signal.

The metering code (lines 106-121 and 133-148) captures channel magnitudes before and after routing, but only for channels whose editors are visible (performance optimisation).

### channel-operations: What operations does the matrix support?

**Answer:** The routing matrix supports **additive send** only. For each source channel, it can target one destination channel. The operation is always addition (`FloatVectorOperations::add`), never replacement or move.

Key characteristics:
- One send target per source channel (via `getSendForSourceChannel`)
- The source signal is preserved (not zeroed)
- Multiple sources can target the same destination (signals sum)
- `setOnlyEnablingAllowed(false)` in the constructor means full routing control (not just channel enable/disable)

The routing matrix also has a separate "connection" concept (source-to-destination mapping) managed by the base RoutingMatrix class for the standard channel routing. The send routing in `renderWholeBuffer` is the additional channel-duplication feature.

### bypass-behaviour: What happens when bypassed?

**Answer:** The `setSoftBypass()` override (RouteFX.h:89) is empty - it does nothing. The `isFadeOutPending()` override (line 91-94) always returns false. This means:

- Bypass has no effect on the routing. The module cannot be meaningfully soft-bypassed.
- There is no ramp or crossfade when bypass state changes.
- The standard `renderWholeBuffer()` is always called regardless of bypass state.

This makes sense architecturally - the routing matrix handles channel distribution that other modules may depend on. Allowing bypass could break downstream signal flow.

### multichannel: How many channels does the matrix support?

**Answer:** The routing matrix supports up to `NUM_MAX_CHANNELS` channels (a compile-time constant, typically 16 in HISE). The `renderWholeBuffer()` method iterates `b.getNumChannels()` which reflects the actual channel count of the processing buffer. The metering arrays use `NUM_MAX_CHANNELS` (line 108, 135).

The constructor sets `getMatrix().setOnlyEnablingAllowed(false)`, giving full routing control over all available channels.

## Processing Chain Detail

1. **Pre-routing metering** - captures per-channel magnitude. Only for visible editor channels. CPU: negligible.
2. **Channel send routing** - for each source channel with a send target, adds source to target. CPU: negligible (one vector add per active send).
3. **Post-routing metering** - captures per-channel magnitude. Only for visible editor channels. CPU: negligible.

## CPU Assessment

- **Baseline:** negligible - a few vector additions per block, proportional to the number of active sends
- **Scaling factors:** Number of active send routes scales cost, but each is a single vector add operation
- **No per-sample processing** - all operations are block-level

## UI Components

The editor class is `RouteFXEditor` (created in RouteFX.cpp:85-98). This displays the routing matrix UI. No FloatingTile content types discovered.

## Notes

- RouteFX is one of the simplest modules in the codebase - zero parameters, zero modulation chains, and the DSP is just additive channel copies.
- The empty `applyEffect()` override is required by the base class interface but all actual work happens in `renderWholeBuffer()`. This is because routing needs access to all channels simultaneously, not just the stereo pair that `applyEffect()` typically receives.
- The module is closely related to SendFX and SendContainer, but operates at a different level: RouteFX duplicates channels within the same processing buffer, while SendFX routes signal to a completely separate module's buffer.
- The `setOnlyEnablingAllowed(false)` call in the constructor distinguishes this from modules where the routing matrix is just for channel enable/disable. Here, full source-to-destination and send routing is available.
