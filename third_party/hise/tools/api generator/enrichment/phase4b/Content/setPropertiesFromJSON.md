Content::setPropertiesFromJSON(String name, var jsonData) -> void

Thread safety: UNSAFE
Sets multiple properties on a component identified by name using a JSON object. Each
key must match a valid component property name (e.g., text, width, height, x, y).
Throws a script error if the named component does not exist.

Pair with:
  getComponent -- retrieve the component reference for direct property access
  componentExists -- check if the component exists before setting properties

Anti-patterns:
  - Invalid property keys in the JSON are silently ignored -- no error reported for
    misspelled property names.

Source:
  ScriptingApiContent.cpp  Content::setPropertiesFromJSON()
    -> looks up component by name -> iterates JSON properties
    -> calls setScriptObjectProperty() for each valid key
