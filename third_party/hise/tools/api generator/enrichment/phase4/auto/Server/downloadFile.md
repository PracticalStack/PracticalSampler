Starts a background file download from the given sub-URL and returns a `Download` object. The `targetFile` parameter must be a `File` object pointing to a file (not a directory). The callback fires periodically during the download with no parameters - instead, query the download state through the `this` keyword inside the callback, which refers to the `Download` object.

The `this.data` object provides these properties:

| Property | Type | Description |
|----------|------|-------------|
| `finished` | bool | `false` while downloading. Set to `true` exactly once when the download completes or is paused. |
| `success` | bool | When `finished` is `true`, indicates whether the download completed successfully. `false` if interrupted or paused. |
| `numTotal` | int | Total file size in bytes. Does not change during the download. |
| `numDownloaded` | int | Number of bytes downloaded so far. |

You can also call any `Download` class method on `this` inside the callback (e.g. `this.getProgress()`, `this.isRunning()`, `this.stop()`).

If you call this method again with the same URL and parameters, the existing download's callback is replaced rather than starting a second download. If the target file already exists, the download resumes from where it left off - delete the file first if you want a fresh download.

> [!Warning:Target file deleted on download start] The target file is deleted and overwritten when the download begins. Do not point it at an existing file you want to keep unless you intend to resume a previous download of the same URL.
