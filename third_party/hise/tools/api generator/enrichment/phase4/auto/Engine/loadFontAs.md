Loads a font file from the project's Images folder and registers it under the given font ID. Place font files inside the Images directory (a subfolder like `{PROJECT_FOLDER}Fonts/` within Images is fine). Use the registered ID in component properties and `Engine.setGlobalFont()`.

```js
Engine.loadFontAs("{PROJECT_FOLDER}Fonts/MyFont.ttf", "MyFont");
Engine.setGlobalFont("MyFont");
```

> [!Warning:Font files must be in Images folder] Font files must be in the Images folder, not the project root. The `{PROJECT_FOLDER}` prefix resolves to Images.