File::startAsProcess(String parameters) -> Integer

Thread safety: UNSAFE -- launches an external OS process (I/O, potential blocking).
Launches this file as an external process with command-line parameters. For
executables, starts the program; for documents, opens with the default app.
Returns true if the process was successfully launched.

Anti-patterns:
  - Only reports whether the process launched, not whether it completed successfully.
    The launched process runs independently.
  - Pass empty string for no parameters.

Source:
  ScriptingApiObjects.cpp  ScriptFile::startAsProcess()
    -> f.startAsProcess(parameters)
