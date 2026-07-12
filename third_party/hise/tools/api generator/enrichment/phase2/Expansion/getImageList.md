## getImageList

**Examples:**

```javascript:expansion-image-list
// Title: Swapping a background image in the expansion callback
// Context: When the active expansion changes, load the first available
// image from the expansion as the interface background.

const var Background = Content.getComponent("Background");
const var expHandler = Engine.createExpansionHandler();

inline function onExpansionChanged(e)
{
    if (isDefined(e))
    {
        local images = e.getImageList();

        if (images.length > 0)
            Background.set("fileName", images[0]);
    }
    else
    {
        Background.set("fileName", "{PROJECT_FOLDER}background.png");
    }
}

expHandler.setExpansionCallback(onExpansionChanged);
onExpansionChanged(undefined);
```

```json:testMetadata:expansion-image-list
{
  "testable": false,
  "skipReason": "Requires installed expansion packs with image files"
}
```
