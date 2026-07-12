File::rename(String newName) -> Integer

Thread safety: UNSAFE -- performs filesystem I/O (file rename operation).
Renames this file to the given name within the same directory. The original file
extension is always preserved -- if newName includes an extension, it is replaced.

Dispatch/mechanics:
  f.getSiblingFile(newName).withFileExtension(f.getFileExtension())
  -> f.moveFileTo(newFile)

Pair with:
  move -- for cross-directory relocation
  getParentDirectory/getChildFile -- to get a handle to the renamed file

Anti-patterns:
  - The original extension is always preserved. rename("data.txt") on a .json file
    produces "data.json", not "data.txt". Use move() to change the extension.
  - After renaming, the File object still references the old filename. The internal
    path is immutable. Get a new handle via getParentDirectory().getChildFile("newName").

Source:
  ScriptingApiObjects.cpp  ScriptFile::rename()
    -> f.getSiblingFile(newName).withFileExtension(f.getFileExtension())
    -> f.moveFileTo(newFile)
