Restores the full processor state from a Base64 string previously obtained via `Effect.exportState()`. This briefly suspends audio processing and kills all active voices while the state is being loaded.

> [!Warning:Resync UI components after restore] Connected UI components (knobs, sliders) do not update automatically after a state restore. Call `updateValueFromProcessorConnection()` on each connected component to resync the display.
