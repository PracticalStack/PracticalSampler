Attaches the broadcaster to audio engine processing specification changes, firing when the sample rate or buffer size changes (e.g. when the DAW changes audio settings or during offline rendering).

> [!Warning:No initial values on attachment] No initial values are dispatched on attachment. Listeners do not receive the current sample rate and buffer size until the next audio engine reinitialisation. Call `resendLastMessage` if you need the current values immediately.
