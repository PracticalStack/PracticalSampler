Reads the file and parses its content as JSON. Returns `undefined` if the file does not exist. On first plugin launch the settings file will not exist, so always guard with `isDefined()` or check `isFile()` before accessing properties on the result.

> [!Warning:Throws error on invalid JSON] Reports a script error if the file exists but contains invalid JSON. This differs from `loadEncryptedObject` and `loadFromXmlFile`, which silently return `undefined` on parse failure.
