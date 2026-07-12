File::writeAsXmlFile(var jsonData, String tagName) -> Integer

Thread safety: UNSAFE -- performs filesystem I/O (file write operation).
Converts a JSON object to XML format and writes to this file. The tagName
becomes the root XML element. Returns true on success.

Dispatch/mechanics:
  ValueTreeConverters::convertDynamicObjectToValueTree(jsonData, Identifier(tagName))
  -> v.createXml()->createDocument("") -> writeString(s)

Pair with:
  loadFromXmlFile -- inverse operation (XML to JSON)

Anti-patterns:
  - Round-trip through ValueTree may not preserve all JSON structures identically.
    Test with complex data structures before relying on fidelity.
  - The tagName must be a valid XML identifier. Invalid characters produce
    malformed XML.

Source:
  ScriptingApiObjects.cpp  ScriptFile::writeAsXmlFile()
    -> ValueTreeConverters::convertDynamicObjectToValueTree
    -> createXml()->createDocument("") -> writeString
