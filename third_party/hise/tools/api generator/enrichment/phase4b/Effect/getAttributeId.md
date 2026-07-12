Effect::getAttributeId(Number index) -> String

Thread safety: WARNING -- String return involves atomic ref-count operations.
Returns the name of the parameter at the given index as a string. Useful for
building dynamic UIs or debugging parameter mappings.
Required setup:
  const var fx = Synth.getEffect("MyEffect");
Pair with:
  getAttributeIndex -- reverse lookup (name to index)
  getNumAttributes -- total parameter count
Source:
  ScriptingApiObjects.h:1971  ScriptingEffect::getAttributeId()
    -> effect->getIdentifierForParameterIndex(index).toString()
