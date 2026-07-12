Console::blink() -> undefined

Thread safety: SAFE
Sends a visual flash to the HISE code editor at the calling line. Lightweight indicator that a code path was reached, without halting execution. Only works in the IDE with `HISE_USE_NEW_CODE_EDITOR` enabled. Dispatched asynchronously to the message thread.

Anti-patterns:
- Only works if the code editor is displaying the file containing the `blink()` call. If a different file is open, the blink is silently ignored.
