File::getRelativePathFrom(ScriptObject otherFile) -> String

Thread safety: SAFE -- no I/O, computes relative path from in-memory path strings.
Returns the relative path from the given base directory to this file. Path
separators are normalized to forward slashes for cross-platform compatibility.

Anti-patterns:
  - The base file must be a directory. Passing a file reports "otherFile is not a
    directory".
  - Passing a non-File object (e.g., a string path) reports "otherFile is not a
    file". Use FileSystem.fromAbsolutePath() to create a File first.

Source:
  ScriptingApiObjects.cpp  ScriptFile::getRelativePathFrom()
    -> dynamic_cast<ScriptFile*>(otherFile.getObject())
    -> f.getRelativePathFrom(sf->f).replaceCharacter('\\', '/')
