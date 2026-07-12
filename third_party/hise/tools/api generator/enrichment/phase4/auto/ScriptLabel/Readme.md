<!-- Diagram triage:
  - none (no diagrams in api_reference.json)
-->
# ScriptLabel

ScriptLabel is an editable text label component for interface text and lightweight text input. It stores its value as a String in the `text` property, so use `set("text", ...)` or `setValue()` to update it and `getValue()` to read it. Use it for static headings, inline naming, or quick search fields where a full text editor would be overkill. For editable labels, call `setEditable()` in `onInit` and react to user edits via a control callback, and enable `updateEachKey` only when you need live filtering as the user types. Preset saving stores the current text value, so pair `saveInPreset` with your intended persistence for transient inputs.

Key ScriptLabel properties include:
- `fontName`, `fontSize`, `fontStyle` (including the `Password` style for obscured text)
- `alignment` for text justification
- `editable` and `multiline` for input behaviour
- `updateEachKey` for per-key callbacks while editing

```js
const var lb = Content.addLabel("Title", 10, 10);
```

> [!Tip:String values only, numeric helpers disabled] Only string values are supported for value updates, and numeric helpers like normalised or undo-based setters are disabled for ScriptLabel.

## Common Mistakes

- **Use editable property at design time**
  **Wrong:** `lb.setEditable(false);` in `onControl`
  **Right:** `lb.setEditable(false);` in `onInit`
  *setEditable is onInit only and reports a script error after initialisation.*

- **Label value is text not numeric**
  **Wrong:** `lb.setValue(1);`
  **Right:** `lb.setValue("1");`
  *ScriptLabel only accepts strings for setValue; non-string values are ignored.*

- **Use changed to trigger callback manually**
  **Wrong:** Set label text programmatically and expect callbacks to fire
  **Right:** Call `changed()` after updating the text
  *Programmatic updates do not trigger the control callback unless you call changed().*
