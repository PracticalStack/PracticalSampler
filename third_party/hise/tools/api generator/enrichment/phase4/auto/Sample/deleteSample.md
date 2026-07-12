Removes this sample from the sampler's sample map. The deletion is deferred - all playing voices are stopped first, then the sound is removed asynchronously.

> [!Warning:Reference invalid after deletion] After calling `deleteSample()`, this Sample reference is invalid. Discard it immediately and do not use it in further loop iterations - any subsequent method call throws "Sound does not exist".
