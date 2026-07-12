SlotFX::setEffect(var effectName) -> ScriptObject

Thread safety: UNSAFE -- suspends audio processing, kills voices, acquires processing chain lock. Creates new processor via factory and asynchronously deletes the old one.
Loads a new effect into the slot by type name. Returns an Effect handle in
HotswappableProcessor mode, or a DspNetwork object in DspNetwork mode. If the
same effect type is already loaded, returns immediately without reloading.
Required setup:
  const var slot = Synth.getSlotFX("MyEffectSlot");
Dispatch/mechanics:
  HotswappableProcessor:
    ScopedTicket + killVoicesAndExtendTimeOut + LockHelpers::freeToGo
    -> EffectProcessorChainFactoryType::createProcessor(typeName)
    -> prepareToPlay, setParentProcessor
    -> LOCK_PROCESSING_CHAIN: swap wrappedEffect
    -> GlobalAsyncModuleHandler::removeAsync(old)
    -> if JavascriptProcessor: auto-compile
    -> return new ScriptingEffect(currentEffect)
  DspNetwork::Holder:
    if same network active: return existing DspNetwork
    -> clearAllNetworks() -> getOrCreate(effectName)
    -> return DspNetwork var
Pair with:
  clear -- to reset slot to passthrough
  getCurrentEffectId -- check before calling to avoid redundant reloads
  getModuleList -- to discover valid effect names
Anti-patterns:
  - [BUG] If the effect name is not found in HotswappableProcessor mode, the slot
    is silently cleared (loaded with EmptyFX) and returns an Effect handle wrapping
    EmptyFX. No error is reported.
  - Do NOT assume the return type without knowing the slot mode -- Effect for
    HotswappableProcessor, DspNetwork for scriptnode-based slots.
  - In DspNetwork mode, setEffect() clears ALL previously loaded networks. Any
    references to previously loaded DspNetwork objects become invalid.
  - Do NOT look up the loaded effect by internal child name
    (Synth.getEffect("SlotId_EffectType")) -- use the returned handle directly.
Source:
  ScriptingApiObjects.cpp:3738  ScriptingSlotFX::setEffect()
    -> SuspendHelpers::ScopedTicket + killVoicesAndExtendTimeOut
    -> slot->setEffect(effectName, false)
  SlotFX.cpp  SlotFX::setEffect()
    -> LockHelpers::freeToGo, LOCK_PROCESSING_CHAIN
    -> factory createProcessor, prepareToPlay, async delete old
