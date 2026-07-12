Exports a JSON object to a file. Relative paths are resolved against the UserPresets directory. Use `Engine.loadFromJSON()` to read the file back.

> [!Warning:Only accepts JSON objects, not arrays] Only JSON objects are accepted. Passing an Array causes a script error. To export an array, wrap it in an object: `Engine.dumpAsJSON({"data": myArray}, "file.json")`.