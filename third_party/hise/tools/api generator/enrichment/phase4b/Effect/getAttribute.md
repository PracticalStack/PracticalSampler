Effect::getAttribute(Number index) -> Double

Thread safety: SAFE
Returns the current value of the parameter at the given index. Use the effect's
named constants for readable index access (e.g., fx.Frequency instead of 0).
Required setup:
  const var fx = Synth.getEffect("MyEffect");
Pair with:
  setAttribute -- write the value back
  getAttributeId -- get the parameter name for a given index
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::getAttribute()
    -> effect->getAttribute(index)
