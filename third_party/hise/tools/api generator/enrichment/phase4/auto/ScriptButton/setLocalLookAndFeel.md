Attaches a scripted look and feel object to this component and all its children. Pass `false` to clear it. The relevant LAF function is `drawToggleButton`.

> [!Warning:LAF overrides filmstrip rendering] A custom LAF takes priority over filmstrip rendering. If you have both a `filmstripImage` property and a local look and feel, only the LAF drawing is used.
