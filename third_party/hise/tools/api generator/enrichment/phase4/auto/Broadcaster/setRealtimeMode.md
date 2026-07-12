Enables lock-free realtime-safe dispatch for broadcasters that receive synchronous messages from the audio thread. When enabled, synchronous sends skip lock acquisition and iterate listeners without copying arguments.

All listeners added to a realtime-safe broadcaster are validated for audio-thread safety. In exported plugins, only inline functions are accepted.

> [!Warning:Listener list must stay stable] Realtime mode removes thread-safety protections. The broadcaster's listener list must be stable and only modified during initialisation, not while the audio thread is running.
