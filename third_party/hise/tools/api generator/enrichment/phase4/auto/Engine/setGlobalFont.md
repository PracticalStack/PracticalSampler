Sets the default font used throughout the plugin for labels, combo boxes, and other UI elements. The font must have been previously loaded via `Engine.loadFontAs()`. Pass an empty string to reset to the default HISE font.

```js
Engine.loadFontAs("{PROJECT_FOLDER}Fonts/MyFont.ttf", "MyFont");
Engine.setGlobalFont("MyFont");
```