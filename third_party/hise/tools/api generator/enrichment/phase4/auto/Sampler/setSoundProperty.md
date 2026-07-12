Sets a sound property for a specific sound in the legacy script selection. Part of the legacy selection API - prefer `Sample` objects from `createSelection()` for new code.

> [!Warning:Parameter order reversed from getter] The parameter order is reversed compared to `getSoundProperty(propertyIndex, soundIndex)` - this method takes `soundIndex` first, then `propertyIndex`. This inconsistency can cause hard-to-diagnose bugs.
