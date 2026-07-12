Replaces the audio file(s) on disk with the provided buffer array. The array must contain one Buffer per channel across all mic positions, matching the structure returned by `loadIntoBufferArray()`. All buffers must have the same length. Returns `true` on success.

> [!Warning:No thread-safety locks acquired] Does not acquire any thread-safety locks internally. Ensure no voices are playing the sample before calling this method.
