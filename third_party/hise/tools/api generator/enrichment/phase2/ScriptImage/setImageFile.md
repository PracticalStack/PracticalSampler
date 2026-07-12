## setImageFile

**Examples:**

```javascript:expansion-image-swap
// Title: Swapping background image based on active expansion
// Context: Plugins with expansion packs swap artwork dynamically
// when the user loads a different expansion

const var img = Content.addImage("BackgroundImage", 0, 0);
img.set("width", 1024);
img.set("height", 768);

// Show default project image on startup
img.setImageFile("{PROJECT_FOLDER}background.png", 0);

const var expHandler = Engine.createExpansionHandler();

inline function onExpansionChanged(e)
{
    if (isDefined(e))
    {
        // Expansion provides its own image list
        local images = e.getImageList();

        if (images.length > 0)
            img.set("fileName", images[0]);
    }
    else
    {
        // No expansion active -- revert to project default
        img.set("fileName", "{PROJECT_FOLDER}background.png");
    }
};

expHandler.setExpansionCallback(onExpansionChanged);
```
```json:testMetadata:expansion-image-swap
{
  "testable": false,
  "skipReason": "Requires image files in the project's Images folder and loaded expansion packs"
}
```

The `{PROJECT_FOLDER}` prefix resolves to the project's `Images/` folder. For expansion images, use the reference returned by `Expansion.getWildcardReference()` or `Expansion.getImageList()` instead of constructing paths manually.
