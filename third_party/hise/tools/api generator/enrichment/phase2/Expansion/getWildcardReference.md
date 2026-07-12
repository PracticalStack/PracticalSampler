## getWildcardReference

**Examples:**

```javascript:expansion-image-swap
// Title: Expansion-specific image loading with project fallback
// Context: Swap background and panel images when the active expansion
// changes. Fall back to project folder images when no expansion is active.

const var BackgroundImage = Content.getComponent("BackgroundImage");
const var expHandler = Engine.createExpansionHandler();

inline function onExpansionChanged(newExpansion)
{
    local bgRef = "";

    if (isDefined(newExpansion))
    {
        // Build a wildcard reference: {EXP::ExpansionName}background.png
        bgRef = newExpansion.getWildcardReference("background.png");
    }
    else
    {
        // No expansion active -- use the project's own image
        bgRef = "{PROJECT_FOLDER}background.png";
    }

    BackgroundImage.set("fileName", bgRef);
}

expHandler.setExpansionCallback(onExpansionChanged);

// Initialise with no expansion
onExpansionChanged(undefined);
```

```json:testMetadata:expansion-image-swap
{
  "testable": false,
  "skipReason": "Requires installed expansion packs with image files"
}
```

**Pitfalls:**
- The wildcard reference is a string concatenation of `{EXP::Name}` and the relative path. It does not verify the file exists -- passing a wrong path produces a valid-looking reference that silently fails to load.
