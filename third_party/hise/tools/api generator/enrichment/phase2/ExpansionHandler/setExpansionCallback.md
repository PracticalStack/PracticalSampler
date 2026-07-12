## setExpansionCallback

**Examples:**

```javascript:swap-background-images
// Title: Swap background images when expansion changes
// Context: Each expansion provides its own background and panel images
// via getWildcardReference(). When no expansion is active, fall back
// to the project's default images.

const var eh = Engine.createExpansionHandler();
const var BackgroundImage = Content.getComponent("BackgroundImage");
const var ContentPanel = Content.getComponent("ContentPanel");

inline function onExpansionLoaded(e)
{
    local bgPath = "";
    local panelPath = "";

    if (isDefined(e))
    {
        // Expansion-specific images via wildcard reference
        bgPath = e.getWildcardReference("background.png");
        panelPath = e.getWildcardReference("panel_bg.png");
    }
    else
    {
        // No expansion active - use project defaults
        bgPath = "{PROJECT_FOLDER}background.png";
        panelPath = "{PROJECT_FOLDER}panel_bg.png";
    }

    BackgroundImage.set("fileName", bgPath);

    ContentPanel.loadImage(panelPath, "bg");
    ContentPanel.setImage("bg", 0, 0);
    ContentPanel.repaint();
};

eh.setExpansionCallback(onExpansionLoaded);

// Initialize the UI for the no-expansion state
onExpansionLoaded(undefined);
```
```json:testMetadata:swap-background-images
{
  "testable": false,
  "skipReason": "Requires image files and UI components (BackgroundImage, ContentPanel) that cannot be created via script API"
}
```

```javascript:update-image-from-expansion
// Title: Update image list from active expansion
// Context: Simpler pattern - swap a single image component's source
// based on the active expansion's image list.

const var eh = Engine.createExpansionHandler();
const var background = Content.getComponent("background");

inline function onExpansionChanged(e)
{
    if (isDefined(e))
        background.set("fileName", e.getImageList()[0]);
    else
        background.set("fileName", "{PROJECT_FOLDER}background.png");
};

eh.setExpansionCallback(onExpansionChanged);

// Set the default state before any expansion is loaded
onExpansionChanged(undefined);
```
```json:testMetadata:update-image-from-expansion
{
  "testable": false,
  "skipReason": "Requires image files and UI component (background) that cannot be created via script API"
}
```

**Pitfalls:**
- The callback does not fire at registration time. If you need the UI to reflect the current state immediately (typically the "no expansion" default), call your callback function manually with `undefined` after registering it.
