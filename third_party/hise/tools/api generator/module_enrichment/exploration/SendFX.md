# SendFX - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/effects/fx/RouteFX.h` (505 lines) - SendEffect lines 236-500
- `hi_core/hi_modules/effects/fx/RouteFX.cpp` (158 lines) - SendEffect::createMetadata lines 35-68

## Gap Answers

### signal-flow

**Question:** How does the SendEffect route audio? Does it pass through or divert?

**Answer:** The SendEffect is a pure send - it does not modify the input buffer. In `applyEffect()` (RouteFX.h:414-444), it calls `container->addSendSignal(b, ...)` which additively copies the signal into the container's internal buffer. The original audio buffer `b` is untouched. The dry signal passes through the effect chain unmodified.

The effect also has `hasTail() = false` and `isSuspendedOnSilence() = true` (lines 324-326), meaning it produces no tail and is suspended when the input is silent.

### gain-smoothing-detail

**Question:** How does the Smoothing toggle affect gain changes?

**Answer:** The gain uses `juce::SmoothedValue<float>` (RouteFX.h:488). In `prepareToPlay()` (line 447-457), the smoothing is configured with `gain.reset(blockRate, 0.08)` - that's 80ms ramp time at the block rate (not audio rate).

When Smoothing is On (default), `applyEffect()` reads `gain.getCurrentValue()` and `gain.getNextValue()` to get start and end gain for the block, creating a per-block ramp via `addFromWithRamp`.

When Smoothing is Off, both start and end gain are set to `gain.getTargetValue()` (the instantaneous target), so gain changes take effect immediately with no ramp. This can cause clicks on sudden gain changes.

### send-index-connection

**Question:** How does SendIndex connect to a specific SendContainer?

**Answer:** The `connect()` method (RouteFX.h:465-486) scans all SendContainer instances in the main synth chain using `ProcessorHelpers::getListOfAllProcessors<SendContainer>()`. The SendIndex is 1-based: index 0 means disconnected (container set to nullptr), index 1 connects to the first SendContainer, index 2 to the second, etc.

The connection is stored as a `WeakReference<SendContainer>` (line 496), so if the container is deleted, the reference safely becomes nullptr.

The editor (RouteFX.h:363-368) populates a combo box with the IDs of all available SendContainers, so users see container names, not raw indices.

### bypass-behaviour

**Question:** What happens when the SendEffect is bypassed?

**Answer:** The `setSoftBypass()` override (RouteFX.h:459-463) sets a `shouldBeBypassed` flag. In `applyEffect()` (lines 435-441):
- If `wasBypassed` is true, `thisGain` (start gain) is forced to 0
- If `shouldBeBypassed` is true, `nextGain` (end gain) is forced to 0
- `wasBypassed` is then updated to `shouldBeBypassed`

This creates a smooth ramp to/from zero over one block when bypass state changes, preventing clicks. The transition is always one block long regardless of the Smoothing toggle.

### mod-chain-application

**Question:** How is the Send Modulation chain applied?

**Answer:** In `applyEffect()` (RouteFX.h:429-433), the modulation chain is sampled twice per block:
- `startModValue` = modulation value at `startSample`
- `endModValue` = modulation value at `startSample + numSamples - 1`

These are multiplied into the start/end gain values respectively:
```
thisGain *= startModValue
nextGain *= endModValue
```

The result is a per-block ramped gain that incorporates both the parameter gain and the modulation chain output. This is not per-sample modulation - it's a linear ramp between two modulated gain values per block.

## Additional Findings

- The default gain is -100 dB, which converts to effectively zero linear gain. Users must raise the gain to hear any send signal.
- The gain SmoothedValue operates at block rate, not audio rate (reset with `blockRate = sampleRate / samplesPerBlock`).
- If `sendIndex != 0` but the container reference is null (e.g. after a failed connection), `prepareToPlay` attempts to reconnect (line 455-456).
- The modulation metadata uses `ScaleOnly` parameter mode (RouteFX.cpp:67), meaning the modulation chain output scales the gain rather than replacing it.

## Issues

No description inaccuracies found. All base data descriptions match the C++ implementation.
