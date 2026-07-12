SlotFX::swap(var otherSlot) -> Integer

Thread safety: UNSAFE -- acquires MainController lock for the atomic exchange.
Atomically exchanges the loaded effects between this slot and another SlotFX
instance. Both slots' internal state (effect reference, type index, clear flag)
is swapped under lock. After swapping, both slots send rebuild messages to update
their UIs. Only works between HotswappableProcessor-mode SlotFX instances.
Required setup:
  const var slotA = Synth.getSlotFX("EffectSlot1");
  const var slotB = Synth.getSlotFX("EffectSlot2");
  slotA.setEffect("SimpleReverb");
  slotB.setEffect("Delay");
Dispatch/mechanics:
  ScopedLock(MainController::getLock())
    -> exchange wrappedEffect, currentIndex, isClear between both slots
    -> sendRebuildMessage(true) on both
    -> sendOtherChangeMessage(ProcessorChangeEvent::Any) on both
Pair with:
  setEffect -- to load effects before swapping
  getCurrentEffect -- to get the effect handle after swap (previous handles reflect new positions)
Anti-patterns:
  - Not supported in DspNetwork::Holder mode -- throws script error
    ("Source Slot is invalid") if the source slot is scriptnode-based.
  - [BUG] Only works between two SlotFX module instances. If the other slot wraps
    a HardcodedSwappableEffect (which also implements HotswappableProcessor), the
    swap silently returns false without an error message.
Source:
  SlotFX.cpp  SlotFX::swap()
    -> dynamic_cast<SlotFX*>(otherSwap)
    -> ScopedLock(getMainController()->getLock())
    -> exchange wrappedEffect, currentIndex, isClear
    -> sendRebuildMessage(true) on both slots
