Effect::getNumAttributes() -> Integer

Thread safety: SAFE
Returns the total number of parameters exposed by the wrapped effect module.
This count matches the number of named constants available on the Effect handle.
Required setup:
  const var fx = Synth.getEffect("MyEffect");
Pair with:
  getAttribute -- read parameter by index
  getAttributeId -- get parameter name by index
Source:
  ScriptingApiObjects.h:1971  ScriptingEffect::getNumAttributes()
    -> effect->getNumParameters()
