Initiates an OS-level file drag operation from this panel. Accepts a file path string, a `File` object, or an array of either. The `moveOriginal` parameter controls whether the file is moved (`1`) or copied (`0`). The optional finish callback is called when the drag operation completes.

On Windows the drag runs synchronously, blocking the message thread until the user drops or cancels.
