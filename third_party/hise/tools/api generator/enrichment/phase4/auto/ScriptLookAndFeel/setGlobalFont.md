Sets the font used for built-in rendering of alert windows, popup menus, combo boxes, text buttons, and dialog buttons. The font is resolved via `Engine.loadFontAs()` aliases or system font names.

> [!Warning:Does not affect paint callbacks] This method only controls fonts rendered by built-in component drawing. Fonts inside `registerFunction()` paint callbacks are unaffected - use `g.setFont()` directly in each callback.
