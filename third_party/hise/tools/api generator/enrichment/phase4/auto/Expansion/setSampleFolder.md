Redirects this expansion's Samples folder to a new location by creating a link file. The preferred way to set up sample locations is during installation via `ExpansionHandler.installFromPackage()`. Use this method as a fallback to let users fix an incorrect sample path without editing files manually.

The method does not move any samples to the new location - the user must do this separately. It is recommended to hint to the user that they should restart the plugin after changing the sample folder, as cached paths from the old location may still be in use.

Returns `true` if the link file was created. Returns `false` if the target folder matches the current sample folder (no change needed).

> [!Warning:Requires a File object, not a string] Pass a `File` object from `FileSystem.getFolder()` or a file browser dialog. Passing a string path silently returns `false`.
