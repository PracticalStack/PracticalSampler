Switches from the default preset data model (which serialises all `saveInPreset` component values automatically) to a custom model where your script callbacks handle save and load. The save callback must return a JSON object describing the full plugin state; the load callback receives that object and restores the state.

The `usePersistentObject` flag is useful during development: when true, it calls the save callback before recompiling and the load callback afterward, so your custom state survives recompilation. Disable it when first defining your data object, since the persisted state may overwrite your initial values.

> [!Warning:Preset name argument unreliable] The save callback's preset name argument is not reliable - it may contain a placeholder value. Do not use it for generating file paths or display names.
