Installs an expansion from an `.hr` archive package file. The installation runs asynchronously on the sample loading thread (voices are killed first). It decompresses the archive, creates the expansion directory structure, and reinitialises all expansions when finished. If credentials have been set via `setCredentials()`, the intermediate file is automatically encrypted to `.hxp`; otherwise it is saved as `.hxi`.

The second parameter controls where samples are placed:

| Value | Behaviour |
|---|---|
| `FileSystem.Expansions` | Samples are copied into the expansion folder (inside the AppData directory). |
| `FileSystem.Samples` | Samples are copied to the global sample folder specified in the Custom Settings panel. A symlink file is created in the expansion's sample folder to redirect to that location. |
| Custom `File` object | Samples go to the specified folder. A symlink file is created in the expansion's sample folder pointing to this custom location. Use this for user-configurable sample storage across multiple drives. |

You can query or change the sample location after installation using `Expansion.setSampleFolder()` and `Expansion.getSampleFolder()`.

Track installation progress with `setInstallCallback()`. Alternatively, `ScriptPanel.setLoadingCallback()` provides a simpler progress indicator for the extraction phase, since it hooks into the same sample loading thread.

> [!Warning:Invalid sample directory fails silently] If the second parameter is neither a `FileSystem` constant nor a valid `File` object, the method reports "The sample directory does not exist" via the error function and the installation does not proceed.
