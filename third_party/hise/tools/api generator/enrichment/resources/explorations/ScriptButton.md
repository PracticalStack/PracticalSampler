# ScriptButton -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/explorations/ScriptComponent_base.md` -- base class infrastructure
- `enrichment/phase1/Content/Readme.md` -- factory class (Content.addButton)
- `enrichment/resources/survey/class_survey_data.json` -- ScriptButton entry
- `enrichment/base/ScriptButton.json` -- base method list (35 methods)

## Source Files

- **Header:** `hi_scripting/scripting/api/ScriptingApiContent.h` (lines 1158-1233)
- **Implementation:** `hi_scripting/scripting/api/ScriptingApiContent.cpp` (lines 2711-2822)
- **Component Wrapper:** `hi_scripting/scripting/api/ScriptComponentWrappers.cpp` (lines 1245-1365)
- **Wrapper Header:** `hi_scripting/scripting/api/ScriptComponentWrappers.h` (lines 540-561)
- **Underlying JUCE Component:** `hi_core/hi_core/MacroControlledComponents.h` (HiToggleButton, line 491)
- **MomentaryToggleButton:** `hi_core/hi_core/MacroControlledComponents.h` (line 474)
- **FilmstripLookAndFeel:** `hi_tools/hi_tools/HI_LookAndFeels.h` (line 582)

## Class Declaration

```cpp
struct ScriptButton : public ScriptComponent
{
    enum Properties
    {
        filmstripImage = ScriptComponent::Properties::numProperties,
        numStrips,
        isVertical,
        scaleFactor,
        radioGroup,
        isMomentary,
        enableMidiLearn,
        setValueOnClick,
        mouseCursor,
        numProperties
    };

    // Constructor / Destructor
    ScriptButton(ProcessorWithScriptingContent *base, Content *parentContent,
                 Identifier name, int x, int y, int, int);
    ~ScriptButton();

    // Overrides
    static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptButton") }
    Identifier getObjectName() const override { return getStaticObjectName(); }
    bool isAutomatable() const override { return true; }
    ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
    const Image getImage() const { return image ? *image.getData() : Image(); }
    void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue,
        NotificationType notifyEditor = sendNotification) override;
    StringArray getOptionsFor(const Identifier &id) override;
    void handleDefaultDeactivatedProperties() override;

    ValueToTextConverter getValueToTextConverter() const override
    {
        return ValueToTextConverter::createForOptions({ "Off", "On" });
    }

    // API Methods (ScriptButton-specific -- only 1)
    /** Sets a FloatingTile that is used as popup. */
    void setPopupData(var jsonData, var position);

    // Non-API overrides
    void resetValueToDefault() override
    {
        setValue((int)getScriptObjectProperty(defaultValue));
    }

    Rectangle<int> getPopupPosition() const;
    const var& getPopupData() const;

private:
    struct Wrapper;
    Rectangle<int> popupPosition;
    var popupData;
    PooledImage image;
};
```

**Key observations:**
- ScriptButton inherits DIRECTLY from ScriptComponent (no intermediary)
- Has exactly ONE ScriptButton-specific API method: `setPopupData`
- All 34 other methods are inherited from ScriptComponent
- `isAutomatable()` returns `true` -- can be a plugin parameter
- Private state: `popupPosition`, `popupData`, `image` (PooledImage for filmstrip)

## Constructor Analysis

```cpp
ScriptButton::ScriptButton(...) : ScriptComponent(base, name)
{
    // Property registration with type selectors
    ADD_SCRIPT_PROPERTY(i00, "filmstripImage");  ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
    ADD_NUMBER_PROPERTY(i01, "numStrips");
    ADD_SCRIPT_PROPERTY(i02, "isVertical");      ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_NUMBER_PROPERTY(i03, "scaleFactor");
    ADD_NUMBER_PROPERTY(i05, "radioGroup");
    ADD_SCRIPT_PROPERTY(i04, "isMomentary");     ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i06, "enableMidiLearn"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i07, "setValueOnClick"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i08, "mouseCursor");     ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

    handleDefaultDeactivatedProperties();

    // Default values
    setDefaultValue(ScriptComponent::Properties::x, x);
    setDefaultValue(ScriptComponent::Properties::y, y);
    setDefaultValue(ScriptComponent::Properties::width, 128);
    setDefaultValue(ScriptComponent::Properties::height, 28);
    setDefaultValue(ScriptButton::Properties::filmstripImage, "");
    setDefaultValue(ScriptButton::Properties::numStrips, "2");
    setDefaultValue(ScriptButton::Properties::isVertical, true);
    setDefaultValue(ScriptButton::Properties::scaleFactor, 1.0f);
    setDefaultValue(ScriptButton::Properties::radioGroup, 0);
    setDefaultValue(ScriptButton::Properties::isMomentary, 0);
    setDefaultValue(ScriptButton::Properties::enableMidiLearn, true);
    setDefaultValue(ScriptButton::Properties::setValueOnClick, false);
    setDefaultValue(ScriptButton::Properties::mouseCursor, "ParentCursor");

    initInternalPropertyFromValueTreeOrDefault(filmstripImage);

    ADD_API_METHOD_2(setPopupData);
}
```

**Key observations:**
- Default dimensions: 128 x 28 pixels
- Only `ADD_API_METHOD_2` (non-typed) for `setPopupData` -- no ADD_TYPED_API_METHOD_N
- `initInternalPropertyFromValueTreeOrDefault(filmstripImage)` -- ensures image is loaded from saved state during restore
- Default numStrips is "2" (stored as string, later parsed to int)

## ScriptButton-Specific Properties

| Property | Default | Type Selector | Description |
|---|---|---|---|
| `filmstripImage` | `""` | FileSelector | Path to a filmstrip image from the Images pool |
| `numStrips` | `"2"` | (number) | Number of frames in the filmstrip (2 or 6 supported) |
| `isVertical` | `true` | ToggleSelector | Whether the filmstrip is arranged vertically |
| `scaleFactor` | `1.0` | (number) | Scale factor for filmstrip rendering |
| `radioGroup` | `0` | (number) | Radio group ID (0 = no group; same non-zero ID = mutual exclusion) |
| `isMomentary` | `0` | ToggleSelector | If true, button is ON only while pressed (returns to OFF on release) |
| `enableMidiLearn` | `true` | ToggleSelector | Whether the button can be MIDI-learned |
| `setValueOnClick` | `false` | ToggleSelector | If true, triggers value change on mouse DOWN instead of mouse UP |
| `mouseCursor` | `"ParentCursor"` | ChoiceSelector | Mouse cursor style (see valid values below) |

## Deactivated Properties

```cpp
void ScriptButton::handleDefaultDeactivatedProperties()
{
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));
    deactivatedProperties.removeAllInstancesOf(getIdFor(ScriptComponent::Properties::isPluginParameter));
}
```

- `min` and `max` are DEACTIVATED -- buttons have a fixed 0/1 range
- `isPluginParameter` is explicitly ACTIVATED (removed from deactivated list) -- buttons can be host-automated

## Value Model

- ScriptButton value is binary: 0 (off) or 1 (on)
- `getValue()` / `setValue()` are inherited from ScriptComponent (no override)
- `getValueNormalized()` / `setValueNormalized()` are inherited (no override) -- for a 0/1 button these are identity operations
- `resetValueToDefault()` is overridden to cast through `(int)getScriptObjectProperty(defaultValue)` -- ensures integer comparison
- `ValueToTextConverter` returns `{"Off", "On"}` -- for host automation display

## Method Registration Summary

The Wrapper struct for ScriptButton:

```cpp
struct ScriptingApi::Content::ScriptButton::Wrapper
{
    API_VOID_METHOD_WRAPPER_2(ScriptButton, setPopupData);
};
```

Only 1 method is registered via `ADD_API_METHOD_2`: `setPopupData`.
All 34 other methods come from the ScriptComponent base class via `ADD_API_METHOD_*` in the ScriptComponent constructor (see ScriptComponent_base.md).

**There are ZERO `ADD_TYPED_API_METHOD_N` registrations in ScriptButton.** The single method uses the non-typed `ADD_API_METHOD_2`.

## setPopupData Implementation

```cpp
void ScriptButton::setPopupData(var jsonData, var position)
{
    popupData = jsonData;

    Result r = Result::ok();
    popupPosition = ApiHelpers::getIntRectangleFromVar(position, &r);

    if (r.failed())
    {
        throw String("position must be an array with this structure: [x, y, w, h]");
    }
}
```

- `jsonData`: A JSON object describing a FloatingTile configuration (passed directly to `FloatingTile` constructor)
- `position`: Must be a `[x, y, w, h]` array. Parsed via `ApiHelpers::getIntRectangleFromVar` which accepts either an Array of 4 ints or a RectangleDynamicObject. Throws a script error if the format is wrong.
- The popup data is stored and used by HiToggleButton to show a FloatingTile popup on click

## Popup Behavior (HiToggleButton::mouseDown)

When a button has popupData set (`.isObject()` is true):
1. On left-click, if the button is NOT already inside a FloatingTilePopup:
   - If a popup is currently showing (`currentPopup != nullptr`), it closes the popup
   - Otherwise, creates a new `FloatingTile` with the stored JSON data, sizes it to popupPosition dimensions, and shows it as a root popup at the popupPosition offset relative to the button

The popup acts as a toggle: click once to show, click again to dismiss.

## filmstripImage Property Handler

```cpp
void ScriptButton::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, ...)
{
    if (id == getIdFor(filmstripImage))
    {
        if (newValue == "Use default skin" || newValue == "")
        {
            setScriptObjectProperty(filmstripImage, "");
            image.clear();
        }
        else
        {
            setScriptObjectProperty(filmstripImage, newValue);
            PoolReference ref(getProcessor()->getMainController(), newValue.toString(),
                             ProjectHandler::SubDirectories::Images);
            image = getProcessor()->getMainController()->getExpansionHandler().loadImageReference(ref);
        }
    }
    ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}
```

- Images are loaded from the project's Images pool via the ExpansionHandler (supports expansion pack images)
- Setting to empty string `""` or `"Use default skin"` clears the image
- The `PooledImage` reference type is used (reference-counted pool lookup)

## getOptionsFor Override

```cpp
StringArray ScriptButton::getOptionsFor(const Identifier &id)
{
    if (id == getIdFor(filmstripImage))
    {
        StringArray sa;
        sa.add("Load new File");
        sa.add("Use default skin");
        sa.addArray(getProcessor()->getMainController()->getCurrentImagePool()->getIdList());
        return sa;
    }
    if (id == getIdFor(mouseCursor))
        return ApiHelpers::getMouseCursorNames();

    return ScriptComponent::getOptionsFor(id);
}
```

Options for filmstripImage: `["Load new File", "Use default skin", ...pool image IDs...]`
Options for mouseCursor: see the mouse cursor values table below.

## Mouse Cursor Values

The `mouseCursor` property accepts these string values (from `ApiHelpers::getMouseCursorNames()`):

| Value | Description |
|---|---|
| `"ParentCursor"` | Inherits cursor from parent component (DEFAULT) |
| `"NoCursor"` | Invisible cursor |
| `"NormalCursor"` | Standard arrow cursor |
| `"WaitCursor"` | Hourglass / busy cursor |
| `"IBeamCursor"` | Text I-beam cursor |
| `"CrosshairCursor"` | Crosshair cursor |
| `"CopyingCursor"` | Arrow with "+" for copy operations |
| `"PointingHandCursor"` | Hand with pointing finger |
| `"DraggingHandCursor"` | Open hand for dragging |
| `"LeftRightResizeCursor"` | Left-right arrow |
| `"UpDownResizeCursor"` | Up-down arrow |
| `"UpDownLeftRightResizeCursor"` | Four-direction arrow |
| `"TopEdgeResizeCursor"` | Top edge resize |
| `"BottomEdgeResizeCursor"` | Bottom edge resize |
| `"LeftEdgeResizeCursor"` | Left edge resize |
| `"RightEdgeResizeCursor"` | Right edge resize |
| `"TopLeftCornerResizeCursor"` | Top-left corner resize |
| `"TopRightCornerResizeCursor"` | Top-right corner resize |
| `"BottomLeftCornerResizeCursor"` | Bottom-left corner resize |
| `"BottomRightCornerResizeCursor"` | Bottom-right corner resize |

When set to `"ParentCursor"`, the wrapper traverses up the parent chain looking for a ScriptPanel with a custom mouse cursor set, and uses that cursor. If no parent panel has a custom cursor, the default cursor is used.

## Filmstrip Rendering (FilmstripLookAndFeel)

The `FilmstripLookAndFeel` class handles filmstrip-based button rendering:

- **2-strip mode:** Index 0 = off, index 1 = on. Simple toggle.
- **6-strip mode:** 6 frames arranged as:
  - Index 0: normal/off
  - Index 1: normal/on
  - Index 2: pressed/off
  - Index 3: pressed/on
  - Index 4: hover/off
  - Index 5: hover/on
- If `isVertical` is true, frames are stacked vertically (each frame has full image width, height = total height / numStrips)
- If `isVertical` is false, frames are arranged horizontally (each frame has full image height, width = total width / numStrips)
- `scaleFactor` scales the rendered output size
- If the image is invalid or numStrips is not 2 or 6, falls back to `GlobalHiseLookAndFeel::drawToggleButton`

## Momentary Button Behavior (MomentaryToggleButton)

When `isMomentary` is true:
- `mouseDown`: Sets toggle state to true (ON) with notification
- `mouseUp`: Sets toggle state to false (OFF) with notification
- Right-click is ignored (bypassed in both mouseDown and mouseUp)
- The button is only ON while the mouse button is physically held down

When `isMomentary` is false (default):
- Normal JUCE ToggleButton behavior (click toggles state)

## Radio Group Behavior

When `radioGroup` is set to a non-zero value:
- JUCE's `setRadioGroupId()` is called on the underlying HiToggleButton
- All buttons in the same radio group automatically enforce mutual exclusion -- only one can be ON at a time
- When used as plugin parameters, buttons with non-zero radioGroup are automatically marked as `isMeta = true` in `ScriptedControlAudioParameter` (because changing one button affects others in the group)

## setValueOnClick Property

When `setValueOnClick` is true:
- Calls `b->setTriggeredOnMouseDown(true)` on the underlying JUCE button
- The button fires its value change on mouse DOWN instead of the default mouse UP
- Useful for immediate response (no delay waiting for mouse release)

## enableMidiLearn Property

When `enableMidiLearn` is true (default):
- Right-clicking the button shows the MIDI learn popup
- `b->setCanBeMidiLearned(newValue)` is called on the underlying component

Note: The `saveInPreset` property ALSO affects MIDI learn capability -- the wrapper's switch statement has `PROPERTY_CASE::ScriptComponent::saveInPreset: b->setCanBeMidiLearned(newValue)`, meaning if `saveInPreset` is set to false, MIDI learn is also disabled regardless of `enableMidiLearn`.

## ButtonWrapper Property Update Switch

The ButtonWrapper handles these property change cases:

| Property | Action |
|---|---|
| `saveInPreset` | `setCanBeMidiLearned(newValue)` |
| `useUndoManager` | `setUseUndoManagerForEvents(...)` |
| `text` | `setButtonText(...)` |
| `enabled` | `enableMacroControlledComponent(...)` |
| `tooltip` | `setTooltip(...)` |
| `enableMidiLearn` | `setCanBeMidiLearned(newValue)` |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | `updateColours(b)` |
| `filmstripImage`, `numStrips`, `scaleFactor` | `updateFilmstrip(b, sc)` |
| `radioGroup` | `setRadioGroupId(...)` |
| `isMomentary` | `setIsMomentary(...)` |
| `setValueOnClick` | `setTriggeredOnMouseDown(...)` |
| `mouseCursor` | `setMouseCursor(ApiHelpers::getMouseCursorFromString(...))` |

## Colour Mapping

```cpp
void ButtonWrapper::updateColours(HiToggleButton * b)
{
    b->setColour(HiseColourScheme::ComponentTextColourId, GET_OBJECT_COLOUR(textColour));
    b->setColour(HiseColourScheme::ComponentOutlineColourId, GET_OBJECT_COLOUR(bgColour));
    b->setColour(HiseColourScheme::ComponentFillTopColourId, GET_OBJECT_COLOUR(itemColour));
    b->setColour(HiseColourScheme::ComponentFillBottomColourId, GET_OBJECT_COLOUR(itemColour2));
}
```

| Script Property | JUCE Colour ID | Meaning |
|---|---|---|
| `bgColour` | ComponentOutlineColourId | Background / outline colour |
| `itemColour` | ComponentFillTopColourId | Fill colour (top gradient) |
| `itemColour2` | ComponentFillBottomColourId | Fill colour (bottom gradient) |
| `textColour` | ComponentTextColourId | Text label colour |

## LAF (Look and Feel) Integration

ScriptButton uses the `drawToggleButton` LAF function. The LAF callback object provides:

| Property | Type | Description |
|---|---|---|
| `id` | String | Component ID |
| `area` | Array [x, y, w, h] | Local bounds |
| `enabled` | bool | Whether enabled |
| `text` | String | Button text |
| `over` | bool | Mouse hover state |
| `down` | bool | Mouse pressed state |
| `value` | bool | Toggle state (on/off) |
| `bgColour` | int (ARGB) | Background colour |
| `itemColour1` | int (ARGB) | First item colour |
| `itemColour2` | int (ARGB) | Second item colour |
| `textColour` | int (ARGB) | Text colour |
| `parentType` | String | Parent FloatingTile content type (if any) |

When a ScriptedLookAndFeel is attached (via `setLocalLookAndFeel`), the `drawToggleButton` function completely replaces the default rendering. The filmstrip rendering is part of the JUCE LookAndFeel system, so a custom LAF takes priority over filmstrip settings.

## Virtual Method Overrides Summary

ScriptButton overrides these virtual methods from ScriptComponent:

| Method | Override Behavior |
|---|---|
| `getObjectName()` | Returns `"ScriptButton"` |
| `isAutomatable()` | Returns `true` |
| `createComponentWrapper()` | Creates `ButtonWrapper` |
| `setScriptObjectPropertyWithChangeMessage()` | Custom filmstrip image loading on `filmstripImage` change |
| `getOptionsFor()` | Custom options for `filmstripImage` (image pool) and `mouseCursor` |
| `handleDefaultDeactivatedProperties()` | Deactivates `min`/`max`, activates `isPluginParameter` |
| `getValueToTextConverter()` | Returns `{"Off", "On"}` converter |
| `resetValueToDefault()` | Casts default value to int before calling setValue |

ScriptButton does NOT override: `getValue()`, `setValue()`, `setValueNormalized()`, `getValueNormalized()`, `changed()`, `sendRepaintMessage()`.

## No Preprocessor Guards

ScriptButton has no conditional compilation (`#if USE_BACKEND` etc.). It is available in all build targets.
