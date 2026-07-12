Extracts this ZIP archive to the specified target directory (either a File object or an absolute path string). The extraction runs asynchronously on the sample loading thread and silences audio output while active. The callback receives a status object at each stage:

| Property | Type | Description |
|----------|------|-------------|
| `Status` | int | `0` = starting, `1` = extracting, `2` = complete |
| `Progress` | double | Overall progress from 0.0 to 1.0 (tracks file count, not bytes) |
| `TotalBytesWritten` | int | Running total of bytes extracted so far |
| `CurrentFile` | String | Relative path of the file being extracted |
| `Cancel` | bool | Set to `true` in the callback to abort extraction |
| `Target` | String | Target directory path |
| `Error` | String | Error message if extraction fails; empty on success |

For archives with fewer than 500 entries, the callback fires for every extracted file. Larger archives throttle callbacks to conserve the scripting queue. To extract to privileged locations on Windows (e.g. the user's VST3 folder), enable the Admin Permissions checkbox in your project preferences.

> [!Warning:Cancel leaves partial files on disk] User cancellation via the `Cancel` flag stops extraction, but files already written remain on disk. There is no automatic rollback.

> [!Warning:Keep File reference alive during extraction] If the File object is garbage collected while extraction is still running, the operation aborts silently. Keep a persistent reference to the File object for the duration of the extraction.
