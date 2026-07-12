Registers a callback that fires synchronously before a user preset is loaded. The callback blocks the load pipeline until it returns, so keep it fast.

What the callback receives depends on whether preprocessing is enabled:

- **Default:** A file reference to the preset file. The preset data loads unchanged regardless of what the callback does.
- **With `setEnableUserPresetPreprocessing`:** A JSON object representing the full preset data. Modifications to this object are applied before loading - enabling version migration, value remapping, and conditional transformations.

You can pass a Broadcaster instead of a plain function when multiple systems need to react before a preset loads (e.g. saving locked parameter values, stopping transport, preparing UI).
