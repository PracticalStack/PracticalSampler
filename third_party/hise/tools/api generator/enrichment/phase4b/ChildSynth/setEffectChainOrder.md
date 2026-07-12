ChildSynth::setEffectChainOrder(bool doPoly, var slotRange, var chainOrder) -> undefined

Thread safety: UNSAFE -- acquires AudioLock internally via LockHelpers::SafeLock during effect chain reordering
Changes the processing order of effects within this synth's effect chain. slotRange
is a [start, end] array defining the dynamic range. chainOrder is an array of indices
within that range. Effects outside the range keep position. Missing effects are bypassed.
Dispatch/mechanics:
  ApiHelpers::getPointFromVar(slotRange) -> Range<int>
    -> dynamic_cast<EffectProcessorChain*>(getChildProcessor(EffectChain))
    -> fx->setFXOrder(false, range, chainOrder)
    -> acquires AudioLock, reorders, bypasses missing effects
Anti-patterns:
  - Do NOT rely on the doPoly parameter -- it is accepted but hardcoded to false
    internally. Only master effect order can be changed regardless of what you pass.
    This is a known bug/limitation in the implementation
Source:
  ScriptingApiObjects.cpp  setEffectChainOrder()
    -> EffectProcessorChain::setFXOrder(false, {p.x, p.y}, chainOrder)
    note: doPoly parameter ignored, hardcoded false
  EffectProcessorChain.h:185  setFXOrder() definition
