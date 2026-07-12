SlotFX::getParameterProperties() -> Array

Thread safety: UNSAFE -- allocates Array and DynamicObject instances for each parameter.
Returns an array of objects describing the parameters of the slot's underlying processor.
Each object has: text (name), defaultValue, min, max, skew, stepSize. In DspNetwork mode,
iterates the root node's parameters. For HardcodedMasterFX, returns compiled factory
properties. For a plain SlotFX module, returns undefined (no parameters of its own).
Required setup:
  const var slot = Synth.getSlotFX("MyEffectSlot");
  slot.setEffect("SomeEffect");
Anti-patterns:
  - Do NOT expect parameter properties from a plain SlotFX module -- returns undefined.
    Parameters belong to the loaded effect, not the slot container. This method is
    primarily useful for HardcodedMasterFX and DspNetwork-based slots.
Source:
  ScriptingApiObjects.cpp:3835  ScriptingSlotFX::getParameterProperties()
    -> HotswappableProcessor: slot->getParameterProperties()
    -> DspNetwork: iterates rn->getParameterFromIndex(i)->data
       -> builds JSON with RangeHelpers::storeDoubleRange(ScriptComponents format)
