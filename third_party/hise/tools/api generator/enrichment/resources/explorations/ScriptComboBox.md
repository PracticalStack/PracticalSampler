# ScriptComboBox -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/explorations/ScriptComponent_base.md` -- Base class exploration
- `enrichment/phase1/Content/Readme.md` -- Content class (factory) prerequisite
- `enrichment/resources/survey/class_survey_data.json` -- ScriptComboBox entry

## Source Files

- **Header:** `hi_scripting/scripting/api/ScriptingApiContent.h` (lines 1235-1296)
- **Implementation:** `hi_scripting/scripting/api/ScriptingApiContent.cpp` (lines 2988-3154)
- **Wrapper (JUCE component):** `hi_scripting/scripting/api/ScriptComponentWrappers.h` (lines 618-643) and `ScriptComponentWrappers.cpp` (lines 1126-1243)
- **Wrappers (NativeFunction):** `hi_scripting/scripting/api/ScriptingApiWrappers.cpp` (lines 690-700, 857-867)
- **SubmenuComboBox (custom popup parsing):** `hi_tools/hi_dev/ZoomableViewport.h` (lines 50-149) and `ZoomableViewport.cpp` (lines 463-677)
- **HiComboBox (JUCE component):** `hi_core/hi_core/MacroControlledComponents.h` (lines 435-472)

## Class Declaration

```cpp
struct ScriptComboBox : public ScriptComponent
{
    enum Properties
    {
        Items = ScriptComponent::numProperties,
        FontName,
        FontSize,
        FontStyle,
        enableMidiLearn,
        popupAlignment,
        useCustomPopup,
        numProperties
    };

    ScriptComboBox(ProcessorWithScriptingContent *base, Content *parentContent,
                   Identifier name, int x, int y, int width, int);
    ~ScriptComboBox() {};

    static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptComboBox"); }

    StringArray getOptionsFor(const Identifier &id) override;
    virtual Identifier getObjectName() const override { return getStaticObjectName(); }
    bool isAutomatable() const override { return true; }
    ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
    void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue,
                                                   NotificationType notifyEditor = sendNotification);
    StringArray getItemList() const;

    void resetValueToDefault() override
    {
        setValue((int)getScriptObjectProperty(defaultValue));
    }

    ValueToTextConverter getValueToTextConverter() const override
    {
        auto sa = StringArray::fromLines(getScriptObjectProperty(Properties::Items).toString());
        sa.removeEmptyStrings();
        return ValueToTextConverter::createForOptions(sa);
    }

    void handleDefaultDeactivatedProperties();
    Array<PropertyWithValue> getLinkProperties() const override;

    // API Methods
    String getItemText() const;
    void addItem(const String &newName);

    struct Wrapper;
};
```

### Key Observations

1. **Inherits directly from ScriptComponent** (not via ComplexDataScriptComponent).
2. **Only 2 class-specific API methods** (`addItem`, `getItemText`). All other 33 methods come from ScriptComponent.
3. **`isAutomatable()` returns `true`** -- combo boxes can be plugin parameters.
4. **`resetValueToDefault()`** casts default to int (appropriate for 1-based item index).
5. **`getValueToTextConverter()`** creates a converter from the items list, allowing host automation displays to show item names.

## Properties (ComboBox-Specific)

Registered in constructor (ScriptingApiContent.cpp lines 2997-3004):

| Property | ID string | Selector Type | Default | Notes |
|----------|-----------|---------------|---------|-------|
| `Items` | `"items"` | MultilineSelector | `""` | Newline-separated item list |
| `FontName` | `"fontName"` | ChoiceSelector | `"Default"` | Font family name |
| `FontSize` | `"fontSize"` | Slider (1-200, step 1) | `13.0f` | Font size in pixels |
| `FontStyle` | `"fontStyle"` | ChoiceSelector | `"plain"` | Font style (Bold, Italic, etc.) |
| `enableMidiLearn` | `"enableMidiLearn"` | ToggleSelector | `false` | Whether MIDI learn is available |
| `popupAlignment` | `"popupAlignment"` | ChoiceSelector | `"bottom"` | Where popup appears |
| `useCustomPopup` | `"useCustomPopup"` | ToggleSelector | `false` | Enable submenu/header/separator parsing |

### Default Value Overrides (from base)

The constructor also overrides these base ScriptComponent defaults:

| Property | Default (base) | Default (ComboBox) |
|----------|---------------|-------------------|
| `width` | (varies) | `128` |
| `height` | (varies) | `32` |
| `defaultValue` | `0` | `1` |
| `min` | `0.0` | `1.0` |

### Deactivated Properties

`handleDefaultDeactivatedProperties()` deactivates:
- `ScriptComponent::Properties::max` -- auto-managed based on item count
- `ScriptComponent::Properties::min` -- fixed at 1

And reactivates:
- `ScriptComponent::Properties::isPluginParameter` -- combo boxes support plugin parameters

### Property Options (`getOptionsFor`)

| Property | Options |
|----------|---------|
| `FontStyle` | Dynamic from JUCE `Font::getAvailableStyles()` |
| `FontName` | `"Default"`, `"Oxygen"`, `"Source Code Pro"`, custom fonts from `MainController`, all system typeface names |
| `popupAlignment` | `"bottom"`, `"top"`, `"topRight"`, `"bottomRight"` |
| All others | Delegates to `ScriptComponent::getOptionsFor()` |

### Link Properties

`getLinkProperties()` extends the base with `Properties::Items`, meaning when components are linked, the items list is synchronized.

## Constructor Method Registration

```cpp
ADD_API_METHOD_1(addItem);
ADD_API_METHOD_0(getItemText);
```

Both use plain `ADD_API_METHOD_N` (not typed). No forced parameter types.

## Wrapper Struct

```cpp
struct ScriptingApi::Content::ScriptComboBox::Wrapper
{
    API_VOID_METHOD_WRAPPER_1(ScriptComboBox, addItem);
    API_METHOD_WRAPPER_0(ScriptComboBox, getItemText);
};
```

## Value Model

### 1-Based Indexing

ScriptComboBox uses **1-based indexing** for its value:

- **Value `1`** = first item in the list
- **Value `0`** = nothing selected (the `text` property is displayed as placeholder)
- **`min` is fixed at `1`**, `max` is auto-managed to equal the item count

The `min` and `max` properties are deactivated in the property editor -- they are managed automatically.

### Items Storage

Items are stored as a **newline-separated string** in the `Items` property. The `getItemList()` method tokenizes by newline and removes empty strings:

```cpp
StringArray ScriptingApi::Content::ScriptComboBox::getItemList() const
{
    String items = getScriptObjectProperty(Items).toString();
    if (items.isEmpty()) return StringArray();
    StringArray sa;
    sa.addTokens(items, "\n", "");
    sa.removeEmptyStrings();
    return sa;
}
```

### addItem Implementation

`addItem()` appends a newline + item name to the existing `Items` property, then increments `max`:

```cpp
void ScriptingApi::Content::ScriptComboBox::addItem(const String &itemName)
{
    String newItemList = getScriptObjectProperty(Items);
    newItemList.append("\n", 1);
    newItemList.append(itemName, 128);
    setScriptObjectProperty(Items, newItemList, sendNotification);

    int size = getScriptObjectProperty(max);
    setScriptObjectProperty(ScriptComponent::Properties::min, 1, dontSendNotification);
    setScriptObjectProperty(ScriptComponent::Properties::max, size + 1, dontSendNotification);
}
```

Key behavior: `addItem` reads the current `max` and increments by 1. It also resets `min` to 1 every time. The item name is truncated to 128 characters.

### setScriptObjectPropertyWithChangeMessage Override

When the `Items` property is set directly (e.g., via `set("items", "Item1\nItem2\nItem3")`), the override auto-updates `max` to match the new item count:

```cpp
void ScriptingApi::Content::ScriptComboBox::setScriptObjectPropertyWithChangeMessage(
    const Identifier &id, var newValue, NotificationType notifyEditor)
{
    if (id == getIdFor(Items))
    {
        setScriptObjectProperty(Items, newValue, sendNotification);
        setScriptObjectProperty(max, getItemList().size(), sendNotification);
    }
    ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}
```

### getItemText Implementation

Returns the currently selected item's text based on the 1-based value:

```cpp
String ScriptingApi::Content::ScriptComboBox::getItemText() const
{
    StringArray items = getItemList();

    auto customPopup = getScriptObjectProperty(Properties::useCustomPopup);

    if(customPopup)
    {
        // Filter out headers (**) and separators (___)
        for(int i = 0; i < items.size(); i++)
        {
            auto s = items[i];
            auto isHeadline = s.startsWith("**");
            auto isSeparator = s.startsWith("___");
            if(isHeadline || isSeparator)
                items.remove(i--);
        }
    }

    if(isPositiveAndBelow((int)value, (items.size()+1)))
    {
        auto itemText = items[(int)value - 1];

        if(customPopup)
            return itemText.fromLastOccurrenceOf("::", false, false);
        else
            return itemText;
    }

    return "No options";
}
```

Key behaviors:
- When `useCustomPopup` is true, headers (`**...`) and separators (`___...`) are filtered out before indexing.
- When `useCustomPopup` is true, submenu prefixes (`Category::ItemName`) are stripped, returning only the part after the last `::`.
- When value is out of range, returns `"No options"`.
- Uses `(int)value - 1` for zero-based array indexing from the 1-based value.

## Custom Popup System (SubmenuComboBox)

When `useCustomPopup` is enabled, the item list supports special formatting parsed by `PopupMenuParser` (defined in `ZoomableViewport.cpp`):

### Special Item Syntax

| Syntax | Effect | Example |
|--------|--------|---------|
| `**HeaderText**` | Section header (non-selectable) | `**Oscillators**` |
| `___` | Visual separator line (non-selectable) | `___` |
| `Category::ItemName` | Creates a submenu named "Category" containing "ItemName" | `Filters::LowPass` |
| `Cat1::Cat2::ItemName` | Nested submenus | `Effects::Reverb::Hall` |
| `~~DisabledItem~~` | Greyed-out disabled item | `~~Unavailable~~` |
| `ItemName|` | Column break after this item | `LastInColumn|` |
| `%SKIP%` | Increment index but do not add item | `%SKIP%` |

### Parsing Logic (PopupMenuParser)

The `PopupMenuParser::getSpecialItemType()` checks for markers:

```cpp
static int getSpecialItemType(const String& item)
{
    int flags = SpecialItem::ItemEntry;
    if(item.contains("~~"))    flags = SpecialItem::Deactivated;
    if(item.contains("___"))   flags = SpecialItem::Separator;
    if(item.contains("**"))    flags = SpecialItem::Header;
    if(item.contains("::"))    flags |= SpecialItem::Sub;
    return flags;
}
```

Note: `Sub` is OR'd (can combine with other types), while `Deactivated`, `Separator`, and `Header` overwrite each other. So `Header` takes precedence over `Deactivated` takes precedence over plain `ItemEntry`, but `Sub` can combine with any.

Headers and separators consume no item index. They are non-selectable.

### Value Model Interaction with Custom Popup

The key insight: headers and separators are **still stored in the `Items` property** (they are lines in the newline-separated string), but they do NOT consume index values. The `getItemText()` method filters them out before indexing. However, the `max` property is set from `getItemList().size()` which includes all lines (including headers/separators). This means:

**When using custom popup, `max` may be larger than the number of selectable items.** The JUCE `SubmenuComboBox` handles this correctly internally because the `parseFromStringArray` method only assigns item IDs to selectable entries (headers/separators get no ID). But the scripting value range `1..max` will have gaps corresponding to header/separator positions.

Actually, looking more carefully: `getItemList()` returns ALL non-empty lines including headers and separators, so `max` is set to the total line count. But the JUCE popup uses `parseFromStringArray` which only assigns IDs to selectable items (skipping headers/separators). The popup menu item IDs are sequential starting from 1, only incrementing for selectable items. So the HiComboBox selected ID will be 1-based among selectable items only.

The `getItemText()` method compensates by filtering out headers and separators before indexing, so `value - 1` correctly indexes into the filtered list of selectable items.

However, the `max` property on the ScriptComponent side is set from `getItemList().size()` which counts ALL lines. This creates a discrepancy: the scripting `max` may be larger than the actual number of selectable items. This is a minor inconsistency, but in practice, JUCE's HiComboBox only reports selected IDs from 1 to N_selectable, so the script value will never exceed N_selectable.

## JUCE Component Wrapper (ComboBoxWrapper)

### Class Hierarchy

```
ScriptCreatedComponentWrapper
  ComboBoxWrapper (also: ComboBoxListener)
```

### Construction

Creates a `HiComboBox` (which inherits `SubmenuComboBox -> ComboBox`). Sets up:
- `setup(getProcessor(), getIndex(), name)` -- connects to macro control system
- Adds ComboBoxListener
- Sets `PopupLookAndFeel`
- Initializes all properties
- Updates value without notification
- Sets mouse cursor from parent panel

### Property Update Handler

The `updateComponent(int propertyIndex, var newValue)` switch handles:

| Property | Action |
|----------|--------|
| `ScriptComponent::tooltip` | `setTooltip(newValue)` |
| `ScriptComponent::useUndoManager` | `setUseUndoManagerForEvents(newValue)` |
| `ScriptComponent::enabled` | `enableMacroControlledComponent(newValue)` |
| `ScriptComponent::text` | `setTextWhenNothingSelected(newValue)` |
| `ScriptComboBox::enableMidiLearn` | `setCanBeMidiLearned(newValue)` |
| `ScriptComponent::bgColour/itemColour/itemColour2/textColour` | `updateColours(cb)` |
| `ScriptComboBox::Items` | `updateItems(cb)` |
| `ScriptComboBox::FontName/FontSize/FontStyle` | `updateFont(getScriptComponent())` |
| `ScriptComboBox::useCustomPopup` | `setUseCustomPopup((bool)newValue)` |
| `ScriptComboBox::popupAlignment` | Sets `"popupAlignment"` on component's JUCE properties |

### `text` Property as Placeholder

The `text` property (inherited from ScriptComponent) serves as the "nothing selected" placeholder text. When `ScriptComponent::text` changes, the wrapper calls `setTextWhenNothingSelected(newValue)` on the HiComboBox. This text is shown when the combo box value is 0 (no selection).

### Color Mapping

| ScriptComponent Color | JUCE Colour ID |
|----------------------|----------------|
| `bgColour` | `HiseColourScheme::ComponentOutlineColourId` |
| `itemColour` | `HiseColourScheme::ComponentFillTopColourId` |
| `itemColour2` | `HiseColourScheme::ComponentFillBottomColourId` |
| `textColour` | `HiseColourScheme::ComponentTextColourId` |

### Font Handling

The `updateFont()` method resolves font names:
- `"Oxygen"` or `"Default"` -> `GLOBAL_FONT()` (or `GLOBAL_BOLD_FONT()` for Bold style)
- `"Source Code Pro"` -> `GLOBAL_MONOSPACE_FONT()`
- Any other name -> tries `MainController::getFont(fontName)` for custom fonts, falls back to system font

### Item Update

`updateItems()` clears the JUCE combo box, adds the item list starting from ID 1 via `addItemList()`, rebuilds the popup menu, and restores the selected ID.

### Value Update

`updateValue()` simply calls `cb->updateValue(dontSendNotification)` which reads the current value from the MacroControlledObject system.

## HiComboBox Class

```cpp
class HiComboBox: public SubmenuComboBox,
                  public ComboBox::Listener,
                  public MacroControlledObject,
                  public ProfiledComponent,
                  public TouchAndHoldComponent
```

- Inherits `SubmenuComboBox` for custom popup menu parsing
- Inherits `MacroControlledObject` for macro control integration
- Has `customPopup` bool field (default false)
- Provides `NormalisableRange<double> getRange()` for host automation
- Has `ValueToTextConverter getValueToTextConverter()` for display

## No Virtual Overrides of ScriptComponent API Methods

ScriptComboBox does NOT override any of the virtual ScriptComponent API methods:
- `getValue()` -- uses base implementation (returns numeric value)
- `setValue()` -- uses base implementation (no special clamping beyond base behavior)
- `setValueNormalized()` -- uses base implementation (calls `setValue(normalizedValue)` directly)
- `getValueNormalized()` -- uses base implementation (returns `getValue()` directly)
- `changed()` -- uses base implementation
- `sendRepaintMessage()` -- uses base implementation

This means the combo box's value normalization is simple linear mapping from 0.0..1.0 to the value range (which is 1..N where N = item count). No special mode-based mapping like ScriptSlider.

## Factory Method

Created via `Content.addComboBox(boxName, x, y)` which calls `addComponent<ScriptComboBox>()`.

## No Constants

No `addConstant()` calls in the constructor.

## No Preprocessor Guards

No `#if USE_BACKEND`, `#if HISE_INCLUDE_LORIS`, or similar conditional compilation in ScriptComboBox-specific code.

## Threading / Lifecycle

No special threading constraints beyond the base ScriptComponent behaviors. The combo box follows the same model:
- Create during onInit only (enforced by Content)
- `setValue()` is thread-safe (async UI update)
- `changed()` cannot be called during onInit

## Integration with ScriptComponentEditBroadcaster

From `ScriptComponentEditBroadcaster.cpp` line 725-727: When editing a combo box in the IDE, the items property can be set through the component editor using `setScriptObjectPropertyWithChangeMessage(Items, items.joinIntoString("\n"))`.

## Integration with DynamicComponentContainer

From `DynamicComponentContainer.cpp` line 421: The mapping `{ "ScriptComboBox", "ComboBox" }` is used for type name resolution in dynamic containers.

## Integration with ScriptingContentOverlay

From `ScriptingContentOverlay.cpp` line 119: New components are created via `createNewComponent<ScriptComboBox>()` in the IDE's component creation UI.

From line 642: ScriptComboBox is included in the MIDI-learnable component types.
