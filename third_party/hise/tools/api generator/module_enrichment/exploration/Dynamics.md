# Dynamics - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/Dynamics.h`, `hi_core/hi_modules/effects/fx/Dynamics.cpp`
**Base class:** `MasterEffectProcessor`

## Signal Path

The signal path in `applyEffect()` (Dynamics.cpp:250-341) is a serial chain of three independently toggled processing stages:

input -> [Gate] -> [Compressor + Makeup] -> [Limiter + Makeup] -> output

Each stage is gated by its Enabled toggle. Disabled stages are fully bypassed (zero CPU). All three stages process the same buffer in series - the output of one becomes the input of the next.

## Gap Answers

### signal-path-order: What is the processing order of the three stages?

**Answer:** Fixed order: Gate first, then Compressor, then Limiter. This is hardcoded in `applyEffect()` (lines 254-341) as three sequential `if` blocks. The order cannot be changed by the user. This matches the standard dynamics processing convention: gate removes noise floor, compressor controls dynamic range, limiter prevents peaks.

### stage-independence: When a stage is disabled, is it fully bypassed?

**Answer:** Yes, fully bypassed with zero CPU cost. Each stage is wrapped in `if (gateEnabled)`, `if (compressorEnabled)`, `if (limiterEnabled || limiterPending)`. When the flag is false, the entire per-sample loop and any post-processing (makeup gain) is skipped. The stages are independent - each reads from and writes to the same audio buffer, so the output of the gate feeds into the compressor, etc.

### makeup-gain-calculation: How are makeup gain values calculated?

**Answer:** Both are calculated in `updateMakeupValues()` (Dynamics.cpp:389-411):

**Compressor makeup:** `gainDb = (1 - ratio) * threshold * -1`. Since `ratio` is stored as the reciprocal (e.g., 0.25 for 4:1), and threshold is negative dB, this produces the theoretical gain reduction at the threshold point. For example, threshold = -20 dB and ratio = 4:1 (stored as 0.25): gainDb = (1 - 0.25) * -(-20) = 15 dB.

**Limiter makeup:** `gainDb = -threshold`. Since the limiter ceiling is negative dB, this simply inverts it. For a -6 dB threshold, makeup is +6 dB.

Both are applied as block-level multiplies after the per-sample processing.

### reduction-metering: How are the Reduction parameters updated?

**Answer:** All three reduction meters use the same pattern within each per-sample loop (e.g., lines 266-271 for gate):

```
const float gR = gate.getGainReduction();
if (gR > gateReduction)
    gateReduction = gR;
else
    gateReduction = gateReduction * 0.9999f;
```

This is peak-hold with exponential decay:
- If the current sample's gain reduction exceeds the stored value, it jumps to the new peak
- Otherwise, the stored value decays by multiplying by 0.9999 each sample
- At 44100 Hz, 0.9999^44100 ~ 0.012, so the meter decays roughly 20 dB per second

The reduction values are read back via `getAttribute()` (lines 178-180) and used for UI metering display.

### limiter-pending-logic: What is the limiterPending flag for?

**Answer:** The `limiterPending` flag enables a one-block crossfade when the limiter is toggled on or off (Dynamics.cpp:308-340). When `setInternalAttribute(LimiterEnabled, ...)` is called:

1. `limiterPending` is set to true if the new state differs from the current state (line 133)
2. In `applyEffect()`, if `limiterPending` is true (lines 310-334):
   - A copy of the input buffer is scaled with a dry ramp (1->0 when enabling, 0->1 when disabling)
   - The limiter processes the original buffer
   - The limited buffer is scaled with the complementary wet ramp
   - Both are summed, creating a one-block crossfade
   - `limiterPending` is reset to false

This prevents clicks when toggling the limiter. The gate and compressor do NOT have this crossfade mechanism - they switch instantly.

### per-sample-processing: Are all stages per-sample?

**Answer:** Yes, all three stages process per-sample in their inner loops (gate: lines 259-275, compressor: lines 283-299, limiter: lines 350-366). Each calls `stage.process(l_, r_)` which is a per-sample stereo processing call from the chunkware SimpleCompressor library.

CPU cost per stage: low. Each stage does level detection + gain computation + gain application per sample. The chunkware algorithms are lightweight digital implementations. With all three stages enabled, total cost is still low-to-medium.

Makeup gain (when enabled) is applied as a block-level multiply after the per-sample loop, adding negligible extra cost.

### compressor-ratio-inversion: Is the user-facing ratio value standard?

**Answer:** Yes. The user sees standard ratio values (1:1 to 32:1). In `setInternalAttribute()` (line 146): `compressor.setRatio(1.0f / newValue)` - the internal library expects the reciprocal, but the user-facing parameter uses the standard convention. In `getAttribute()` (line 177): `return 1.0f / compressor.getRatio()` converts back. So a user setting of 4 means 4:1 compression.

## Processing Chain Detail

1. **Gate** - per-sample noise gate. Gated by GateEnabled. Controlled by GateThreshold, GateAttack, GateRelease. Outputs GateReduction (read-only meter). CPU: low.
2. **Compressor** - per-sample downward compressor. Gated by CompressorEnabled. Controlled by CompressorThreshold, CompressorRatio, CompressorAttack, CompressorRelease. Outputs CompressorReduction (read-only meter). CPU: low.
3. **Compressor Makeup** - block-level gain multiply. Gated by CompressorMakeup toggle AND CompressorEnabled. Auto-calculated from threshold and ratio. CPU: negligible.
4. **Limiter** - per-sample brick-wall limiter with one-block crossfade on enable/disable. Gated by LimiterEnabled. Controlled by LimiterThreshold, LimiterAttack, LimiterRelease. Outputs LimiterReduction (read-only meter). CPU: low.
5. **Limiter Makeup** - block-level gain multiply. Gated by LimiterMakeup toggle AND LimiterEnabled. Auto-calculated from threshold. CPU: negligible.

## Conditional Behaviour

- **GateEnabled**: When Off, the entire gate processing loop is skipped. No crossfade on toggle.
- **CompressorEnabled**: When Off, the entire compressor processing loop and makeup are skipped. No crossfade on toggle.
- **LimiterEnabled**: When toggled, triggers a one-block crossfade via the `limiterPending` mechanism. When Off (after transition), the limiter loop is skipped.
- **CompressorMakeup**: When On, applies auto-calculated makeup gain after compression. The gain is recalculated whenever threshold or ratio changes.
- **LimiterMakeup**: When On, applies auto-calculated makeup gain after limiting. The gain is recalculated whenever threshold changes.

## CPU Assessment

- **Baseline (all stages disabled):** negligible - three boolean checks per block
- **Single stage enabled:** low - one per-sample processing loop
- **All stages enabled:** low to medium - three per-sample loops, each with level detection and gain computation
- **Scaling factors:** Number of enabled stages scales CPU linearly. No parameter makes a single stage more expensive.

## UI Components

The editor class is `DynamicsEditor` (created in Dynamics.cpp:235-248). No FloatingTile content types discovered - standard backend editor panel.

## Notes

- All three stages use the chunkware SimpleCompressor library algorithms. The gate uses `SimpleGate`, the compressor uses `SimpleComp`, and the limiter uses `SimpleLimit`.
- The limiter's crossfade on toggle is unique among the three stages. This is likely because the limiter can introduce significant gain changes that would click if switched instantly, while the gate and compressor are more gradual by nature.
- All thresholds default to 0 dB and all stages default to disabled. A newly added Dynamics module does nothing until at least one stage is enabled and its threshold lowered.
- The compressor ratio default of 1.0 (1:1) means no compression even when enabled, until the ratio is increased.
