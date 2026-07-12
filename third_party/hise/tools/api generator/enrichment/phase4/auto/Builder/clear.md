Removes all modules from the MainSynthChain except the calling script processor. This gives the Builder a clean slate before creating new modules. Always follow `clear()` with your `create()` calls and a final `flush()`.

> [!Warning:Skipped silently during project load] If `clear()` runs on the sample loading thread (during project load), it silently does nothing and logs a console message. Subsequent `create()` calls will then add modules to the existing tree rather than a clean slate. Ensure Builder code only runs from a deliberate compile action, not automatically on load.
