## loadImage

**Examples:**

```javascript:load-background-image
// Title: Loading a background image for display
// Context: The simplest ScriptPanel image use case. Load an image from
// the project's Images folder and display it using setImage(). This
// bypasses the paint routine entirely for efficient static image rendering.

// --- setup ---
Content.addPanel("BackgroundPanel", 0, 0);
// --- end setup ---

const var bgPanel = Content.getComponent("BackgroundPanel");
bgPanel.loadImage("{PROJECT_FOLDER}background.png", "bg");
bgPanel.setImage("bg", 0, 0);
```
```json:testMetadata:load-background-image
{
  "testable": false,
  "skipReason": "Requires background.png image file in the project Images folder"
}
```

```javascript:load-multiple-themed-images
// Title: Loading multiple images for themed backgrounds
// Context: Load several background variants at init time and switch
// between them based on a theme selection. The pretty name alias
// lets you reference images without repeating the full path.

const var bgPanel = Content.addPanel("ThemedBackground", 0, 0);
bgPanel.set("width", 800);
bgPanel.set("height", 600);

const var NUM_THEMES = 4;

for (i = 0; i < NUM_THEMES; i++)
    bgPanel.loadImage("{PROJECT_FOLDER}bg" + i + ".png", "theme" + i);

// Switch to a theme by setting the corresponding image
inline function setTheme(index)
{
    bgPanel.setImage("theme" + index, 0, 0);
};

setTheme(0);
```
```json:testMetadata:load-multiple-themed-images
{
  "testable": false,
  "skipReason": "Requires bg0-bg3.png image files in the project Images folder"
}
```

Images are resolved through the expansion handler, so they can come from the main project's Images folder or from an installed expansion pack. Use the `{PROJECT_FOLDER}` prefix for project images or `{EXP::ExpansionName}` for expansion images.
