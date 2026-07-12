ChildSynth::getChildSynthByIndex(int index) -> ScriptObject

Thread safety: INIT -- restricted to onInit, throws script error at runtime
Returns a ChildSynth reference to the child sound generator at the specified index
within this synth's child processor list. Only works if the wrapped synth is a Chain
(SynthGroup or SynthChain). Returns invalid ChildSynth if index out of range or not a Chain.
Required setup:
  const var parentGroup = Synth.getChildSynth("MyGroup");
Dispatch/mechanics:
  objectsCanBeCreated() check -> dynamic_cast<Chain*>(synth.get())
    -> c->getHandler()->getProcessor(index)
    -> returns new ScriptingSynth wrapping the child
Anti-patterns:
  - Do NOT call outside onInit -- throws script error via reportIllegalCall("getChildSynth()", "onInit")
  - Do NOT assume it works on non-Chain synths (e.g., SineSynth) -- cast to Chain* fails
    silently, returns invalid ChildSynth
Source:
  ScriptingApiObjects.cpp  getChildSynthByIndex()
    -> objectsCanBeCreated() guard
    -> dynamic_cast<Chain*>(synth.get()) -> getHandler()->getProcessor(index)
