# SlotFX - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/SlotFX.h`, `SlotFX.cpp`
**Base class:** `MasterEffectProcessor`, `HotswappableProcessor`

## Signal Path

SlotFX delegates all audio processing to its hosted child effect. `applyEffect()` (SlotFX.h:166-169) is empty. Instead, `renderWholeBuffer()` (SlotFX.cpp:97-125) is overridden to forward the entire buffer to the wrapped effect's `renderWholeBuffer()`. When no effect is loaded (isClear = true), the method returns immediately and audio passes through unchanged.

audio in -> [if effect loaded] child effect renderWholeBuffer() -> audio out
audio in -> [if empty] -> audio out (passthrough)

## Gap Answers

### delegation-mechanism

**Question:** How does SlotFX delegate audio processing?

**Answer:** SlotFX overrides `renderWholeBuffer()` (SlotFX.cpp:97-125) rather than `applyEffect()`. When `isClear` is false and the wrapped effect is not soft-bypassed, it:
1. Calls `wrappedEffect->renderAllChains(0, buffer.getNumSamples())` to process the child's modulation chains
2. Handles multichannel routing: if the buffer has more than 2 channels and the source channels are not 0+1, it creates a 2-channel sub-buffer for the correct channel pair
3. Calls `wrappedEffect->renderWholeBuffer(buffer)` to process the audio

The `applyEffect()` override is empty because the delegation happens at the higher `renderWholeBuffer()` level, which includes modulation chain rendering that `applyEffect()` alone would miss.

### empty-state-behaviour

**Question:** What happens when no effect is loaded?

**Answer:** When `isClear` is true, `renderWholeBuffer()` returns immediately at line 99-100. The audio buffer is not modified, so audio passes through unchanged. The `isClear` flag is set to true when the wrapped effect is null or is an EmptyFX instance (SlotFX.cpp:216). On construction, `clearEffect()` is called which creates an EmptyFX as the default child (SlotFX.h:187).

### swap-mechanism

**Question:** How does effect swapping work?

**Answer:** `setEffect()` (SlotFX.cpp:172-242) creates a new effect, prepares it, then swaps it in under a processing chain lock (`LOCK_PROCESSING_CHAIN`). The old effect is removed asynchronously via `getMainController()->getGlobalAsyncModuleHandler().removeAsync()`. There is no crossfade between the old and new effects - the swap is instantaneous at the next processing boundary (guarded by the lock). The `swap()` method (SlotFX.cpp:127-160) allows two SlotFX instances to exchange their wrapped effects atomically under the main controller lock.

### constrainer-details

**Question:** What effect types can be loaded?

**Answer:** The `Constrainer` class (SlotFX.h:219-248) extends `NoVoiceEffectConstrainer` (which excludes VoiceEffects) and additionally excludes `RouteEffect` and `SlotFX` by type name. The wildcard metadata is `MasterEffect|MonophonicEffect|!RouteEffect|!SlotFX`. So the allowed types are all MasterEffects and MonophonicEffects except RouteEffect and SlotFX itself (no nesting).

### midi-forwarding

**Question:** Does SlotFX forward MIDI events?

**Answer:** Yes. `handleHiseEvent()` (SlotFX.cpp:41-53) forwards MIDI events to the wrapped effect when `isClear` is false and the wrapped effect is not soft-bypassed. Similarly, `startMonophonicVoice()`, `stopMonophonicVoice()`, and `resetMonophonicVoice()` are all forwarded.

### bypass-delegation

**Question:** How does bypass work?

**Answer:** `setSoftBypass()` (SlotFX.h:93-97) delegates directly to the wrapped effect's `setSoftBypass()`, but only if the wrapped effect is not an EmptyFX (checked via `ProcessorHelpers::is<EmptyFX>()`). `updateSoftBypass()` (SlotFX.h:87-91) also delegates to the wrapped effect. `isFadeOutPending()` (SlotFX.h:99-105) delegates to the wrapped effect. So bypass is fully delegated - the SlotFX itself never bypasses, it forwards bypass state to whatever effect is currently hosted.

## Processing Chain Detail

1. **Empty check** (negligible): If `isClear`, return immediately (passthrough)
2. **Bypass check** (negligible): If wrapped effect is soft-bypassed, skip processing
3. **Modulation chain rendering** (delegated): `renderAllChains()` on the wrapped effect
4. **Multichannel routing** (conditional, negligible): Creates sub-buffer for non-standard channel pairs
5. **Effect processing** (delegated): `renderWholeBuffer()` on the wrapped effect - CPU cost depends entirely on which effect is loaded

## Interface Usage

**SlotFX (HotswappableProcessor)**: The core interface. Provides `setEffect()` for loading effects by type name, `clearEffect()` for resetting to EmptyFX, `swap()` for exchanging effects between two slots, `getCurrentEffect()` for accessing the hosted effect, and `getModuleList()` for listing available effect types. The effect list is built at construction time using the constrainer.

## CPU Assessment

- **Overall baseline**: negligible (the SlotFX wrapper itself adds essentially zero overhead)
- **Actual CPU**: entirely determined by the hosted child effect
- **No scaling factors** on the wrapper itself

## UI Components

- Backend editor: `SlotFXEditor` (SlotFX.cpp:27) - provides the effect selection UI

## Notes

SlotFX always has exactly one child processor. When cleared, it creates a new EmptyFX as the child rather than setting the pointer to null. The child's ID is prefixed with the SlotFX's own ID (e.g. "SlotFX_SimpleGain"). If a script effect is loaded, it is automatically compiled after creation. The effect list is populated from the constrainer at construction time and is immutable.
