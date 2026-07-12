Serialises the UI control values of a script modulator to a base64-encoded string. Pair with `restoreScriptControls` to save and restore just the control values without affecting the script itself.

> [!Warning:Only works on script modulators] Only works on script modulators (Script Voice Start Modulator, Script Time Variant Modulator, Script Envelope Modulator). Calling on a built-in modulator type reports a script error.
