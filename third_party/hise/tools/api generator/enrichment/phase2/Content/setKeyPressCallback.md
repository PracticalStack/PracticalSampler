## setKeyPressCallback

**Examples:**

```javascript:keyboard-shortcut-navigation
// Title: Registering keyboard shortcuts for navigation
// Context: Plugins register keyboard shortcuts in a dedicated namespace,
// typically guarded by a user preference. Each shortcut triggers an action
// through the plugin's broadcaster system or by toggling a button.

Content.makeFrontInterface(900, 600);

const var settingsPanel = Content.addPanel("SettingsPanel", 0, 0);
settingsPanel.set("visible", false);
const var mixerPanel = Content.addPanel("MixerPanel", 0, 0);
mixerPanel.set("visible", false);

// Toggle between two main views
Content.setKeyPressCallback("s", function()
{
    settingsPanel.set("visible", !settingsPanel.get("visible"));
});

// Toggle mixer panel visibility
Content.setKeyPressCallback("m", function()
{
    mixerPanel.set("visible", !mixerPanel.get("visible"));
});

// Use modifier keys for less common actions
Content.setKeyPressCallback("shift + f", function()
{
    Console.print("Focus mode toggled");
});

// Unregister a shortcut by passing a non-function value
// Content.setKeyPressCallback("s", 0);
```
```json:testMetadata:keyboard-shortcut-navigation
{
  "testable": false,
  "skipReason": "Key press callbacks require hardware keyboard interaction"
}
```

**Pitfalls:**
- Keyboard shortcut callbacks use plain `function()` syntax (not `inline function`) because they fire on the message thread, not the audio thread. This is correct and expected.
- The callback receives a JSON event object with properties like `description`, `keyCode`, `shift`, `cmd`, `alt`, but for simple shortcuts the callback typically ignores the argument and performs a direct action.
