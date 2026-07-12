Registers a callback that fires after a user preset has finished loading. The callback runs asynchronously on the message thread after all component values, module states, and automation data have been fully restored. It receives a file reference to the loaded preset as its argument. This is the correct place to update UI state after a preset change.

You can pass a Broadcaster instead of a plain function to fan out preset-change notifications to multiple independent listeners.

> [!Warning:Argument undefined for DAW session restore] The callback argument is `undefined` (not a file reference) when the preset was loaded from a DAW session restore rather than a physical file. Guard with `isDefined(presetFile)` before calling methods on it.
