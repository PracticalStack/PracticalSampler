Applies a style configuration object that controls fonts, font size, and colours for the rendered markdown. See `getStyleData()` for the full object format and default values. When multiple renderers share the same visual theme, define a complete style object once and pass it to each renderer.

> [!Warning:Omitted properties reset to defaults] Properties not present in the object are reset to defaults, not preserved. Always start from `getStyleData()` for incremental changes, or define all properties explicitly in a shared style object.
