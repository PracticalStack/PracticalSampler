File::loadFromXmlFile() -> JSON

Thread safety: UNSAFE -- reads file from disk and performs XML parsing plus ValueTree conversion (I/O).
Reads the file as XML, parses into a JUCE ValueTree, and converts to a JSON
object. Returns undefined silently on failure (missing file, invalid XML, empty).

Dispatch/mechanics:
  loadAsString() -> XmlDocument::parse(s)
  -> ValueTree::fromXml(*xml)
  -> ValueTreeConverters::convertValueTreeToDynamicObject(v)

Pair with:
  writeAsXmlFile -- inverse operation (JSON to XML)

Anti-patterns:
  - Returns undefined silently on any failure (no error reported, unlike
    loadAsObject). Always check isDefined().
  - Round-trip fidelity depends on ValueTreeConverters. Not all JSON structures
    survive writeAsXmlFile -> loadFromXmlFile identically.

Source:
  ScriptingApiObjects.cpp  ScriptFile::loadFromXmlFile()
    -> XmlDocument::parse -> ValueTree::fromXml -> convertValueTreeToDynamicObject
