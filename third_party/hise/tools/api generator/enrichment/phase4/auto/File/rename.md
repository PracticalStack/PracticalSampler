Renames this file within the same directory. Returns `true` on success. To reference the renamed file afterwards, obtain a new handle via `getParentDirectory().getChildFile("newName")`.

> [!Warning:Original extension always preserved] The original file extension is always preserved regardless of what `newName` contains. Calling `rename("data.txt")` on a `.json` file produces `data.json`, not `data.txt`. To change the extension, use `move()` with a target File that has the desired extension.
