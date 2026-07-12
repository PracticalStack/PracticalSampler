# setZLevel | UNSAFE

Sets the depth level for this component among its siblings. Reports a script error if the value is not one of the four valid strings (case-sensitive).

```
setZLevel(String zLevel)
```

## Valid Values

| Value | Description |
|-------|-------------|
| `"Back"` | Renders behind all sibling components |
| `"Default"` | Normal rendering order |
| `"Front"` | Renders in front of normal siblings |
| `"AlwaysOnTop"` | Always renders on top of all siblings |

## Source

`ScriptingApiContent.h` line ~1734
