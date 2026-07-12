SlotFX::getCurrentEffect() -> ScriptObject

Thread safety: UNSAFE -- creates a new ScriptingEffect wrapper object (heap allocation) in HotswappableProcessor mode.
Returns a handle to the effect currently loaded in the slot. Returns an Effect
object in HotswappableProcessor mode (including EmptyFX if cleared), or a
DspNetwork object in DspNetwork::Holder mode (undefined if no network active).
Required setup:
  const var slot = Synth.getSlotFX("MyEffectSlot");
  slot.setEffect("SimpleReverb");
Dispatch/mechanics:
  HotswappableProcessor: slot->getCurrentEffect() -> new ScriptingEffect(processor)
  DspNetwork::Holder: holder->getActiveNetwork() -> returns DspNetwork var
Pair with:
  setEffect -- returns the Effect handle directly (preferred over getCurrentEffect)
  getCurrentEffectId -- to check which effect is loaded without creating a wrapper
Anti-patterns:
  - Do NOT assume the return type without knowing the slot mode -- returns Effect
    for HotswappableProcessor, DspNetwork for scriptnode-based slots. Code that
    assumes one type will fail on the other.
Source:
  ScriptingApiObjects.cpp:3779  ScriptingSlotFX::getCurrentEffect()
    -> getSlotFX()->getCurrentEffect() wrapped in ScriptingEffect
    -> or getDspNetworkHolder()->getActiveNetwork()
