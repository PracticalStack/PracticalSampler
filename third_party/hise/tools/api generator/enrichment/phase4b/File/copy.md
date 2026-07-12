File::copy(ScriptObject target) -> Integer

Thread safety: UNSAFE -- performs filesystem I/O (file copy operation).
Copies this file to the location specified by the target File object.
Returns true on success, false otherwise.

Required setup:
  const var source = FileSystem.getFolder(FileSystem.Documents).getChildFile("data.json");
  const var target = FileSystem.getFolder(FileSystem.Documents).getChildFile("data_backup.json");

Dispatch/mechanics:
  dynamic_cast<ScriptFile*>(target) -> f.copyFileTo(sf->f)
  Reports script error "target is not a file" if target is not a File object.

Pair with:
  move -- to relocate instead of duplicate
  copyDirectory -- for recursive directory copying

Anti-patterns:
  - Do NOT pass a string path as target -- must be a File object. Passing a string
    reports a script error "target is not a file".
  - Do NOT use the original File object expecting it to reference the copy -- after
    copying, the original still points to the source path.

Source:
  ScriptingApiObjects.cpp  ScriptFile::copy()
    -> dynamic_cast<ScriptFile*>(target.getObject())
    -> f.copyFileTo(sf->f)
