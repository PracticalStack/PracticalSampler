Checks whether the expansion from a given `.hr` package file is already installed. Reads the archive metadata to determine the target expansion folder, then searches for an existing expansion at that location. Returns the `Expansion` reference if found, `undefined` otherwise.

Use this before calling `installExpansionFromPackage()` to avoid reinstalling an expansion that is already present.

> [!Warning:FileBased expansions are ignored] Only detects Intermediate (`.hxi`) and Encrypted (`.hxp`) expansions. A FileBased expansion at the target location is not considered "installed" by this method.
