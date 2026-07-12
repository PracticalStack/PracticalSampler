Reads the file as XML and converts it to a JSON object. This is the inverse of `writeAsXmlFile`. The root element's tag name becomes the implicit container - attributes become object properties and child elements become nested objects.

> [!Warning:Returns undefined on any failure] Returns `undefined` silently on any failure (missing file, invalid XML, empty content). Unlike `loadAsObject`, no script error is reported. Always guard with `isDefined()`.
