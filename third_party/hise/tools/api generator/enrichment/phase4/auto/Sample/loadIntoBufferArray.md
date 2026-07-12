Loads the complete audio data of this sample into an array of Buffer objects. For multi-mic setups, the array contains buffers for each channel of each mic position in order: `[mic1_L, mic1_R, mic2_L, mic2_R]`. A single mono mic position returns a one-element array.

> [!Warning:Extend timeout for batch processing] Loads the entire sample into memory. When processing many samples in a loop, call `Engine.extendTimeOut()` periodically to prevent the script execution timeout from triggering.
