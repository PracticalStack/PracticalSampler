Writes a JSON data file to this expansion's AdditionalSourceCode directory on disk. The data object is serialised to JSON format and saved to the specified relative path. Returns `true` on success.

> [!Warning:Does not update embedded pool data] For Intermediate and Encrypted expansions, this writes to the filesystem only. The embedded data pool used by `loadDataFile()` is not updated, so subsequent reads on encoded expansions will still return the original pool data until the expansion is reinstalled.
