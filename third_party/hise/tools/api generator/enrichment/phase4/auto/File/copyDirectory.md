Recursively copies this directory and its contents to the target location. The target parameter is the destination directory path itself, not the parent directory into which the copy should be placed. Returns `true` on success.

> [!Warning:Specify exact destination directory] Passing the parent directory as the target overwrites it with the source content. Always specify the actual destination directory path.
