Effect::getAttributeIndex(String id) -> Integer

Thread safety: WARNING -- String parameter comparison involves atomic ref-count operations.
Returns the parameter index for the given parameter name string. Returns -1 if
no parameter with the given name exists.
Required setup:
  const var fx = Synth.getEffect("MyEffect");
Pair with:
  getAttributeId -- reverse lookup (index to name)
Source:
  ScriptingApiObjects.h:1971  ScriptingEffect::getAttributeIndex()
    -> iterates parameter identifiers for string match
