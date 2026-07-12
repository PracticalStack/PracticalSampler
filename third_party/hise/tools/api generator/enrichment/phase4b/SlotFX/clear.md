SlotFX::clear() -> undefined

Thread safety: UNSAFE -- allocates EmptyFX placeholder (HotswappableProcessor mode) or clears network objects (DspNetwork mode). Both paths involve heap operations and audio lock acquisition.
Removes the currently loaded effect and restores the slot to unity-gain passthrough.
In HotswappableProcessor mode, loads an EmptyFX placeholder and asynchronously deletes
the previous effect. In DspNetwork mode, calls clearAllNetworks().
Required setup:
  const var slot = Synth.getSlotFX("MyEffectSlot");
Dispatch/mechanics:
  HotswappableProcessor: LOCK_PROCESSING_CHAIN -> swap wrappedEffect with EmptyFX
    -> GlobalAsyncModuleHandler::removeAsync(oldEffect)
  DspNetwork::Holder: holder->clearAllNetworks()
  Internal isClear flag enables fast-path that skips all processing
Pair with:
  setEffect -- to load a new effect after clearing
  getCurrentEffect -- handle becomes invalid after clear()
Anti-patterns:
  - Do NOT hold references to the previous Effect handle after clear() -- the old
    processor is deleted asynchronously and the handle becomes invalid
  - setEffect("EmptyFX") achieves the same result and is often preferred in
    switch/case logic because it eliminates a special case for "off"
Source:
  SlotFX.cpp  SlotFX::clearEffect()
    -> LOCK_PROCESSING_CHAIN, swap wrappedEffect
    -> GlobalAsyncModuleHandler::removeAsync(old)
    -> new EmptyFX(mc, "Empty"), prepareToPlay, setParentProcessor
