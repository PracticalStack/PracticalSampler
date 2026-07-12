Sample::duplicateSample() -> Sample

Thread safety: UNSAFE -- kills voices, acquires sample lock, suspends audio processing, allocates new sample data, refreshes preload sizes.
Creates a deep copy of this sample within the same sampler. Returns a new Sample
object pointing to the copy. Heavyweight thread synchronization: suspends audio,
kills voices with 1000ms timeout, busy-waits, acquires sample lock, then copies.
Dispatch/mechanics:
  setSyncEditMode -> SuspendHelpers::ScopedTicket -> killVoicesAndExtendTimeOut(1000ms)
    -> busy-waits for audio stop -> acquires SampleLock
    -> copies ValueTree data -> adds to sample map -> refreshes preload sizes
Pair with:
  deleteSample -- remove the original after cloning
Anti-patterns:
  - [BUG] No objectExists() check -- if the underlying sound was deleted,
    dereferences a null pointer instead of reporting "Sound does not exist"
Source:
  ScriptingApiObjects.cpp  duplicateSample()
    -> ScopedValueSetter(syncEditModeFlag)
    -> SuspendHelpers::ScopedTicket
    -> killVoicesAndExtendTimeOut(jp, 1000)
    -> LockHelpers::SafeLock(SampleLock)
    -> copy ValueTree, add to sample map
