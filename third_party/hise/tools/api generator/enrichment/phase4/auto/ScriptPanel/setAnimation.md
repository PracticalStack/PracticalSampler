Loads a Lottie animation from a base64-encoded JSON string. Use the Lottie Developer Panel floating tile to compress animation files to base64. After loading, drive frame progression manually with `setAnimationFrame()` in a timer callback and query state with `getAnimationData()`.

> [!Warning:Requires rLottie, advance frames manually] Requires rLottie to be enabled in the project settings. There is no built-in animation playback - you must advance frames manually using a timer callback.
