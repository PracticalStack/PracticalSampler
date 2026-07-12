# setStyleSheetPseudoState | UNSAFE

Sets one or more CSS pseudo-state selectors on this component. Multiple states can be combined in one string (e.g. `":hover:active"`). Pass an empty string `""` to clear all pseudo-states. Automatically triggers a repaint after setting the state.

```
setStyleSheetPseudoState(String pseudoState)
```

## Valid Pseudo-States

| Selector | Description |
|----------|-------------|
| `:first-child` | First child pseudo-class |
| `:last-child` | Last child pseudo-class |
| `:root` | Root element pseudo-class |
| `:hover` | Mouse hover state |
| `:active` | Active/pressed state |
| `:focus` | Keyboard focus state |
| `:disabled` | Disabled state |
| `:hidden` | Hidden state |
| `:checked` | Checked/toggled state |

## Pair With

- `setStyleSheetClass()` - assign a CSS class to the component
- `setStyleSheetProperty()` - set CSS variable values

## Source

`ScriptingApiContent.h` line ~1734
