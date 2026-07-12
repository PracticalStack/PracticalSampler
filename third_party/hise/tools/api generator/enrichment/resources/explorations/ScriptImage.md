# ScriptImage -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/explorations/ScriptComponent_base.md` -- base class infrastructure
- `enrichment/resources/base_methods/ScriptComponent.md` -- pre-distilled base method entries
- `enrichment/resources/survey/class_survey_data.json` -- ScriptImage entry (lines 1894-1917)

## Source Files

- **Header:** `hi_scripting/scripting/api/ScriptingApiContent.h` lines 1668-1732
- **Implementation:** `hi_scripting/scripting/api/ScriptingApiContent.cpp` lines 4010-4218
- **Component Wrapper:** `hi_scripting/scripting/api/ScriptComponentWrappers.h` line 688, `ScriptComponentWrappers.cpp` lines 2106-2198
- **API Wrappers:** `hi_scripting/scripting/api/ScriptingApiWrappers.cpp` lines 716-738
- **Underlying JUCE Component:** `hi_core/hi_core/MiscComponents.h` lines 660-686, `MiscComponents.cpp` lines 1319-1398

## Class Declaration

```cpp
struct ScriptImage : public ScriptComponent
```

Direct subclass of `ScriptComponent`. No intermediate class. No additional interfaces beyond what ScriptComponent provides.

### Properties Enum

```cpp
enum Properties
{
    Alpha = ScriptComponent::numProperties,  // starts after base properties
    FileName,
    Offset,
    Scale,
    BlendMode,
    AllowCallbacks,
    PopupMenuItems,
    PopupOnRightClick,
    numProperties
};
```

8 ScriptImage-specific properties extending the base ScriptComponent properties.

### Private Members

```cpp
void updateBlendMode();
Image blendImage;
PooledImage image;
gin::BlendMode blendMode = gin::BlendMode::Normal;
```

- `image` -- the loaded `PooledImage` from the image pool
- `blendImage` -- a pre-computed blend result image (used only when blendMode != Normal)
- `blendMode` -- the current blend mode (from the gin library)

## Constructor Analysis

Source: `ScriptingApiContent.cpp` lines 4017-4051

### Property Registration

| Property ID | Macro | Selector Type | Default Value |
|---|---|---|---|
| `alpha` | `ADD_NUMBER_PROPERTY` | Slider (0.0-1.0, step 0.01) | `1.0f` |
| `fileName` | `ADD_SCRIPT_PROPERTY` | `FileSelector` | `""` (empty string) |
| `offset` | `ADD_NUMBER_PROPERTY` | -- | `0` |
| `scale` | `ADD_NUMBER_PROPERTY` | -- | `1.0` |
| `blendMode` | `ADD_SCRIPT_PROPERTY` | `ChoiceSelector` | `"Normal"` |
| `allowCallbacks` | `ADD_SCRIPT_PROPERTY` | `ChoiceSelector` | `false` |
| `popupMenuItems` | `ADD_SCRIPT_PROPERTY` | `MultilineSelector` | `""` |
| `popupOnRightClick` | `ADD_SCRIPT_PROPERTY` | `ToggleSelector` | `true` |

### Base Property Defaults Overridden

- `width` = 50 (base default is set at creation)
- `height` = 50
- `saveInPreset` = `false` (base default is `true`)

### Priority Properties

```cpp
priorityProperties.add(getIdFor(FileName));
```

`FileName` is marked as a priority property (shown first in the property editor).

### API Method Registration

```cpp
ADD_API_METHOD_2(setImageFile);
ADD_API_METHOD_1(setAlpha);
```

Only 2 ScriptImage-specific API methods registered, both using `ADD_API_METHOD_N` (not typed variants).

### Wrapper Struct

```cpp
struct ScriptingApi::Content::ScriptImage::Wrapper
{
    API_VOID_METHOD_WRAPPER_2(ScriptImage, setImageFile);
    API_VOID_METHOD_WRAPPER_1(ScriptImage, setAlpha);
};
```

No `ADD_TYPED_API_METHOD_N` -- both methods use untyped wrappers.

## Deactivated Properties

Source: `handleDefaultDeactivatedProperties()` lines 4191-4204

The following base ScriptComponent properties are deactivated (hidden from the property editor) for ScriptImage:

| Deactivated Property | Reason |
|---|---|
| `bgColour` | Image display, no background colour |
| `itemColour` | Not used |
| `itemColour2` | Not used |
| `max` | Not a range-based control |
| `min` | Not a range-based control |
| `defaultValue` | Not a range-based control |
| `textColour` | No text rendering |
| `macroControl` | Not typically controlled by macros |
| `automationId` | Not typically automated |
| `linkedTo` | Not typically linked |

## Factory Method

Created via `Content.addImage(name, x, y)`:

```cpp
ScriptingApi::Content::ScriptImage * ScriptingApi::Content::addImage(Identifier knobName, int x, int y)
{
    return addComponent<ScriptImage>(knobName, x, y);
};
```

Or via the Interface Designer by adding a "ScriptImage" component. Also created in `createComponentIfTypeMatches` (line 9998).

## setImageFile Implementation

Source: lines 4132-4152

```cpp
void ScriptingApi::Content::ScriptImage::setImageFile(const String &absoluteFileName, bool forceUseRealFile)
{
    ignoreUnused(forceUseRealFile);  // NOTE: parameter is currently ignored!

    CHECK_COPY_AND_RETURN_10(getProcessor());

    if (absoluteFileName.isEmpty())
    {
        image.clear();
        setScriptObjectProperty(FileName, absoluteFileName, sendNotification);
        return;
    }

    PoolReference ref(getProcessor()->getMainController(), absoluteFileName, ProjectHandler::SubDirectories::Images);
    image.clear();
    image = getProcessor()->getMainController()->getExpansionHandler().loadImageReference(ref);

    updateBlendMode();

    setScriptObjectProperty(FileName, absoluteFileName, sendNotification);
};
```

Key observations:
1. **`forceUseRealFile` is ignored** -- the parameter is declared but `ignoreUnused(forceUseRealFile)` is called. The image is always loaded through the expansion handler / image pool.
2. The file name is resolved via `PoolReference` with `ProjectHandler::SubDirectories::Images`, meaning it references the project's `Images/` folder.
3. Loading goes through `ExpansionHandler::loadImageReference()`, which supports both project images and expansion images.
4. Passing an empty string clears the image.
5. After loading, `updateBlendMode()` is called to recompute the blend image if needed.
6. The `FileName` property is updated with notification.

## setAlpha Implementation

Source: line 4186-4189

```cpp
void ScriptingApi::Content::ScriptImage::setAlpha(float newAlphaValue)
{
    setScriptObjectPropertyWithChangeMessage(getIdFor(Alpha), newAlphaValue);
}
```

Simply sets the `Alpha` property with change notification. The actual alpha rendering happens in `ImageComponentWithMouseCallback::paint()`.

## setScriptObjectPropertyWithChangeMessage Override

Source: lines 4111-4130

Handles two special property changes:
1. **FileName** -- calls `setImageFile(newValue, true)` to load the image when the property is set
2. **BlendMode** -- looks up the blend mode index from the options list and casts to `gin::BlendMode`, then calls `updateBlendMode()`

Falls through to `ScriptComponent::setScriptObjectPropertyWithChangeMessage()` for all properties.

## getOptionsFor Override

Source: lines 4059-4107

### FileName Options

Returns "Load new File" followed by all image IDs from `getCurrentImagePool()`.

### AllowCallbacks Options

Delegates to `MouseCallbackComponent::getCallbackLevels()` which returns:

| Index | Value |
|---|---|
| 0 | `"No Callbacks"` |
| 1 | `"Context Menu"` |
| 2 | `"Clicks Only"` |
| 3 | `"Clicks & Hover"` |
| 4 | `"Clicks, Hover & Dragging"` |
| 5 | `"All Callbacks"` |

### BlendMode Options

Returns a hardcoded string array of 24 blend modes:

| Index | Value |
|---|---|
| 0 | `"Normal"` |
| 1 | `"Lighten"` |
| 2 | `"Darken"` |
| 3 | `"Multiply"` |
| 4 | `"Average"` |
| 5 | `"Add"` |
| 6 | `"Subtract"` |
| 7 | `"Difference"` |
| 8 | `"Negation"` |
| 9 | `"Screen"` |
| 10 | `"Exclusion"` |
| 11 | `"Overlay"` |
| 12 | `"SoftLight"` |
| 13 | `"HardLight"` |
| 14 | `"ColorDodge"` |
| 15 | `"ColorBurn"` |
| 16 | `"LinearDodge"` |
| 17 | `"LinearBurn"` |
| 18 | `"LinearLight"` |
| 19 | `"VividLight"` |
| 20 | `"PinLight"` |
| 21 | `"HardMix"` |
| 22 | `"Reflect"` |
| 23 | `"Glow"` |
| 24 | `"Phoenix"` |

These map directly to `gin::BlendMode` enum values.

## Blend Mode Processing

Source: `updateBlendMode()` lines 4206-4218

```cpp
void ScriptingApi::Content::ScriptImage::updateBlendMode()
{
    if (blendMode == gin::Normal)
        return;

    if (image)
    {
        auto original = image->data;
        blendImage = Image(Image::ARGB, original.getWidth(), original.getHeight(), true);
        gin::applyBlend(blendImage, original, blendMode);
    }
}
```

- When blendMode is Normal, no processing occurs
- When a non-Normal blend mode is active, creates a new ARGB image and applies the blend using `gin::applyBlend()`
- The blended result is stored in `blendImage`

### getImage() Logic

```cpp
const Image ScriptingApi::Content::ScriptImage::getImage() const
{
    if (blendMode != gin::Normal)
    {
        return blendImage.isValid() ? blendImage : PoolHelpers::getEmptyImage(...);
    }
    return image ? *image.getData() : PoolHelpers::getEmptyImage(...);
}
```

Returns `blendImage` when a non-Normal blend mode is active, otherwise returns the raw pooled image data. Falls back to an empty image of the component's dimensions if no image is loaded.

## ImageComponentWithMouseCallback -- Paint and Display Logic

Source: `MiscComponents.cpp` lines 1328-1351

```cpp
void ImageComponentWithMouseCallback::paint(Graphics &g)
{
    if (image.isValid())
    {
        if(auto slaf = dynamic_cast<simple_css::StyleSheetLookAndFeel*>(&getLookAndFeel()))
        {
            if(slaf->drawImageOnComponent(g, this, image))
                return;
        }

        g.setOpacity(jmax<float>(0.0f, jmin<float>(1.0f, alpha)));

        // Note: cropArea is computed but not used in the final draw call
        Rectangle<int> cropArea = Rectangle<int>(0,
            (int)((float)offset * scale),
            jmin<int>((int)((float)getWidth() * (float)scale), image.getWidth()),
            jmin<int>((int)((float)getHeight() * (float)scale), image.getHeight()));

        Image croppedImage = image.getClippedImage(cropArea);

        float ratio  = (float)getHeight() / (float)getWidth();
        int heightInImage = (int)((float)image.getWidth() * ratio);
        g.drawImage(image, 0, 0, getWidth(), getHeight(), 0, offset, image.getWidth(), heightInImage);
    }
}
```

Key rendering observations:
1. **CSS override** -- If a `StyleSheetLookAndFeel` is attached, it can intercept the rendering via `drawImageOnComponent()`
2. **Alpha** -- clamped to 0.0-1.0, applied as opacity
3. **Offset** -- used as the Y-offset into the source image (filmstrip-style). The source rectangle starts at `(0, offset)` in the image
4. **Scale** -- used in `cropArea` computation but note that the actual `g.drawImage()` call uses `offset` directly (not scaled). The `croppedImage` variable is computed but not used in the final draw call. The actual rendering uses `image.getWidth()` for the source width and `ratio`-computed height.
5. **Filmstrip behavior** -- The `offset` property selects which vertical strip to display from a tall image. The image is drawn to fill the component bounds, using the aspect ratio of the component to determine how much of the source image height to use.

### Offset/Scale Setters

All simple property setters that trigger `repaint()`:

- `setOffset(int)` -- stores the offset value
- `setScale(double)` -- stores scale with minimum of 0.1
- `setAlpha(float)` -- stores alpha value

## Component Wrapper -- ImageWrapper

Source: `ScriptComponentWrappers.cpp` lines 2106-2198

### Constructor

Creates an `ImageComponentWithMouseCallback` JUCE component. If a CSS `StyleSheetLookAndFeel` is attached, enables mouse click interception and repaint-on-mouse-activity.

### updateComponent() (full refresh)

Reads all ScriptImage properties and applies them to the underlying JUCE component:
- Sets popup menu items and callback level from `AllowCallbacks`
- Sets image, offset, scale, alpha
- If no valid image, shows an empty placeholder image

### updateComponent(int propertyIndex) (single property update)

Switch-based update:
- `AllowCallbacks`, `PopupMenuItems`, `PopupOnRightClick` -> `updatePopupMenu()`
- `FileName`, `Offset`, `Scale`, `Alpha`, `BlendMode` -> `updateImage()`

### Popup Menu System

`getItemList()` (line 4173) parses the `PopupMenuItems` property as newline-separated strings. The popup menu is shown on right-click (or any click depending on `PopupOnRightClick`). The selected item index becomes the component's value.

## Virtual Method Overrides

ScriptImage does NOT override any of the virtual methods from ScriptComponent:
- `getValue()` -- uses base implementation
- `setValue()` -- uses base implementation
- `setValueNormalized()` -- uses base implementation
- `getValueNormalized()` -- uses base implementation
- `changed()` -- uses base implementation
- `sendRepaintMessage()` -- uses base implementation

## Threading / Lifecycle

- No threading constraints beyond what ScriptComponent base provides
- No onInit-only restrictions on ScriptImage-specific methods
- `setImageFile` loads from the image pool, which is a message-thread operation
- Image loading goes through the expansion handler for expansion support

## Preprocessor Guards

None. ScriptImage has no conditional compilation.
