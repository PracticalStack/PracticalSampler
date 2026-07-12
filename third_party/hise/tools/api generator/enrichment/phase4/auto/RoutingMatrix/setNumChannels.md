Sets the number of source channels and relaxes the stereo constraint to allow that many simultaneous connections. Call this before any multichannel `addConnection()` calls. The channel count must be a multiple of 2. Note that this only changes the source channel count and connection limit - the destination channel count is determined by the processor's parent context.

> [!Warning:Not all processors support resizing] Only processors that explicitly allow resizing support this method. Most matrices have resizing disabled and will throw "Can't resize this matrix".
