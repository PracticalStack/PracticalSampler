Loads a JSON data file from this expansion's AdditionalSourceCode directory and returns the parsed object. In normal HISE projects, the AdditionalSourceCode folder is reserved for C++ files, but in expansions it serves as a general-purpose storage area for arbitrary JSON configuration data.

The behaviour depends on the expansion type: FileBased expansions read the file directly from disk, while Intermediate and Encrypted expansions load from the embedded data pool with strong caching. Both paths parse the content as JSON.

Returns `undefined` if the file is not found. Check the return value with `isDefined()` before use.

> [!Warning:JSON parse errors only throw on encoded expansions] On the FileBased path, a missing file silently returns `undefined`. On the Intermediate/Encrypted path, a missing pool reference also returns `undefined`, but malformed JSON throws a script error.
