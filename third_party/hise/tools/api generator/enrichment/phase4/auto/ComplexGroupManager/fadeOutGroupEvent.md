Fades out all currently playing voices that match the specified layer/group combination over the given duration in milliseconds.

> [!Warning:Only affects already-playing voices] Unlike `delayGroupEvent`, `fadeInGroupEvent`, and the other voice start methods, `fadeOutGroupEvent` acts on already-playing voices. Calling it before any matching voices are sounding has no effect.