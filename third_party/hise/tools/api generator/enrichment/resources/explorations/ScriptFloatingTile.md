# ScriptFloatingTile -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/explorations/ScriptComponent_base.md` -- Base class infrastructure
- `enrichment/phase1/Content/Readme.md` -- Content factory and component creation system
- `enrichment/resources/survey/class_survey_data.json` -- Class relationships

## Source Files

- **Header:** `hi_scripting/scripting/api/ScriptingApiContent.h` (lines 2255-2316)
- **Implementation:** `hi_scripting/scripting/api/ScriptingApiContent.cpp` (lines 6092-6167, 7554-7747)
- **Component Wrapper:** `hi_scripting/scripting/api/ScriptComponentWrappers.cpp` (lines 2940-3082)

---

## Class Declaration

```cpp
struct ScriptFloatingTile : public ScriptComponent,
                            public Dispatchable
{
    enum Properties
    {
        itemColour3 = ScriptComponent::Properties::numProperties,
        updateAfterInit,
        ContentType,
        Font,
        FontSize,
        Data,
        numProperties
    };

    // ...

private:
    struct Wrapper;
    var jsonData;
};
```

### Inheritance

- **ScriptComponent** -- standard component base (see ScriptComponent_base.md)
- **Dispatchable** -- lightweight mixin from `hi_core/hi_core/UtilityClasses.h` that provides a weak-referenceable interface for deferred dispatch operations via `LockFreeDispatcher`. Contains a `Status` enum (`OK`, `notExecuted`, `needsToRunAgain`, `cancelled`) and a `Function` typedef. It is a minimal interface -- no methods to implement, just allows the object to be dispatched. The commented-out code in `setScriptObjectPropertyWithChangeMessage` shows a Dispatchable dispatch pattern that was tried but disabled.

---

## Constructor

```cpp
ScriptFloatingTile::ScriptFloatingTile(ProcessorWithScriptingContent *base, Content*, 
                                        Identifier panelName, int x, int y, int, int) :
    ScriptComponent(base, panelName)
{
    ADD_SCRIPT_PROPERTY(i05, "itemColour3");    ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
    ADD_SCRIPT_PROPERTY(i06, "updateAfterInit"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i01, "ContentType");    ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
    ADD_SCRIPT_PROPERTY(i02, "Font");           ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
    ADD_NUMBER_PROPERTY(i03, "FontSize");       ADD_TO_TYPE_SELECTOR(SelectorTypes::SliderSelector);
    ADD_SCRIPT_PROPERTY(i04, "Data");           ADD_TO_TYPE_SELECTOR(SelectorTypes::CodeSelector);

    priorityProperties.add(getIdFor(ContentType));

    setDefaultValue(Properties::itemColour3, 0);
    setDefaultValue(ScriptComponent::Properties::x, x);
    setDefaultValue(ScriptComponent::Properties::y, y);
    setDefaultValue(ScriptComponent::Properties::width, 200);
    setDefaultValue(ScriptComponent::Properties::height, 100);
    setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
    setDefaultValue(Properties::updateAfterInit, true);
    setDefaultValue(Properties::ContentType, EmptyComponent::getPanelId().toString());
    setDefaultValue(Properties::Font, "Default");
    setDefaultValue(Properties::FontSize, 14.0);
    setDefaultValue(Properties::Data, "{\n}");

    handleDefaultDeactivatedProperties();

    ADD_API_METHOD_1(setContentData);
}
```

### Key Constructor Notes

1. **Default dimensions:** 200x100 (unlike most ScriptComponent defaults)
2. **saveInPreset defaults to false** -- unlike most components which default to true. Floating tiles are display/UI elements, not preset-storable controls.
3. **Default ContentType:** `"Empty"` (from `EmptyComponent::getPanelId()`)
4. **Default Font:** `"Default"`, FontSize: 14.0
5. **Default Data:** `"{\n}"` (empty JSON object)
6. **ContentType is a priority property** -- it is resolved first during property application.
7. **Only one API method registered:** `ADD_API_METHOD_1(setContentData)` -- all other methods are inherited from ScriptComponent.

### Wrapper Struct

```cpp
struct ScriptFloatingTile::Wrapper
{
    API_VOID_METHOD_WRAPPER_1(ScriptFloatingTile, setContentData);
};
```

Only one method wrapper. Uses `API_VOID_METHOD_WRAPPER_1` (not typed), so no forced parameter types.

---

## Properties Unique to ScriptFloatingTile

| Property | Type | Default | Selector | Description |
|----------|------|---------|----------|-------------|
| `itemColour3` | Colour | `0` (transparent) | ColourPickerSelector | Fifth colour slot for the floating tile content |
| `updateAfterInit` | Toggle | `true` | ToggleSelector | When true, updating the value triggers a full content reload |
| `ContentType` | Choice | `"Empty"` | ChoiceSelector | The panel type ID string (e.g. "PresetBrowser", "Keyboard") |
| `Font` | Choice | `"Default"` | ChoiceSelector | Font name for text-rendering panels |
| `FontSize` | Number | `14.0` | SliderSelector | Font size for text-rendering panels |
| `Data` | Code | `"{\n}"` | CodeSelector | JSON string for panel-specific configuration |

### Deactivated Properties

The following base ScriptComponent properties are explicitly deactivated (hidden in the property editor, not applicable to floating tiles):

```cpp
void ScriptFloatingTile::handleDefaultDeactivatedProperties()
{
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(saveInPreset));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(macroControl));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(isPluginParameter));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(min));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(max));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(defaultValue));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(pluginParameterName));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(text));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(tooltip));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(useUndoManager));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(processorId));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(parameterId));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(isMetaParameter));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(linkedTo));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(automationId));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(deferControlCallback));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(pluginParameterGroup));
}
```

This is a very aggressive deactivation -- 17 base properties are disabled. The remaining active properties from the base are:
- x, y, width, height (positioning)
- visible, enabled, locked (display state)
- bgColour, itemColour, itemColour2, textColour (colours -- forwarded to floating tile ColourData)
- parentComponent (layout hierarchy)

---

## Method Implementations

### setContentData (THE unique method)

```cpp
void ScriptFloatingTile::setContentData(var data)
{
    jsonData = data;

    if (auto obj = jsonData.getDynamicObject())
    {
        auto id = obj->getProperty("Type");

        // Force content type change notification by clearing first
        setScriptObjectProperty(Properties::ContentType, "", dontSendNotification);
        setScriptObjectProperty(Properties::ContentType, id, sendNotification);
    }
}
```

**Key behavior:**
1. Stores the entire JSON object as `jsonData`
2. Extracts `"Type"` from the JSON and updates `ContentType` property
3. Forces a change notification by first setting ContentType to empty string, then to the actual value -- this ensures the component wrapper receives the update even if the type hasn't changed (useful for reloading with new data)

### setValue / getValue Overrides

```cpp
void ScriptFloatingTile::setValue(var newValue) { value = newValue; }
var ScriptFloatingTile::getValue() const { return value; }
```

**Minimal overrides** -- no validation, no thread safety (no SimpleReadWriteLock), no triggerAsyncUpdate(). The value is just stored directly. This is because floating tiles don't participate in the preset/value system -- they are display components.

### getContentData

```cpp
var ScriptFloatingTile::getContentData() { return jsonData; }
```

Not a scripting API method -- used internally by the component wrapper to get the JSON data for setting up the actual FloatingTile component.

---

## Property Change Handler

The `setScriptObjectPropertyWithChangeMessage` override handles the mapping between ScriptComponent properties and the internal JSON data structure:

```cpp
void ScriptFloatingTile::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor)
{
    if (id == getIdFor(ContentType))
    {
        DynamicObject* obj = createOrGetJSONData();
        obj->setProperty("Type", newValue.toString());
    }
    else if (id == getIdFor(Data))
    {
        var specialData = JSON::parse(newValue.toString());
        if (auto obj = specialData.getDynamicObject())
        {
            auto dataObject = createOrGetJSONData();
            auto prop = obj->getProperties();
            for (int i = 0; i < prop.size(); i++)
                dataObject->setProperty(prop.getName(i), prop.getValueAt(i));
        }
    }
    else if (id == getIdFor(bgColour) || id == getIdFor(textColour) || 
             id == getIdFor(itemColour) || id == getIdFor(itemColour2) || 
             id == getIdFor(itemColour3))
    {
        auto obj = jsonData.getDynamicObject();
        if (obj == nullptr) { obj = new DynamicObject(); jsonData = var(obj); }

        // "Best. Line. Ever." -- renames "itemColour" to "itemColour1"
        Identifier idToUse = id == getIdFor(itemColour) ? Identifier("itemColour1") : id;

        auto colourObj = obj->getProperty("ColourData").getDynamicObject();
        if (colourObj == nullptr) { colourObj = new DynamicObject(); obj->setProperty("ColourData", colourObj); }
        colourObj->setProperty(idToUse, newValue);
    }
    else if (id == getIdFor(Font) || id == getIdFor(FontSize))
    {
        auto obj = createOrGetJSONData();
        obj->setProperty(id, newValue);
    }

    ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}
```

### JSON Data Architecture

The floating tile uses a dual-layer system:
1. **ScriptComponent properties** (the property panel values: ContentType, Font, FontSize, bgColour, etc.)
2. **Internal jsonData** (a `var` holding a `DynamicObject` that represents the complete FloatingTileContent configuration)

When a property changes:
- **ContentType** -> Sets `jsonData["Type"]`
- **Data** -> Parses the JSON string and merges all properties into `jsonData`
- **Colours** -> Creates/updates `jsonData["ColourData"]` sub-object. Note the `itemColour` to `itemColour1` rename.
- **Font/FontSize** -> Sets directly on `jsonData`

The jsonData is the master configuration passed to `FloatingTile::setContent()`.

### createOrGetJSONData

```cpp
DynamicObject* ScriptFloatingTile::createOrGetJSONData()
{
    auto obj = jsonData.getDynamicObject();
    if (obj == nullptr) { obj = new DynamicObject(); jsonData = var(obj); }
    return obj;
}
```

Lazy initialization pattern for the JSON data object.

---

## Component Wrapper (FloatingTileWrapper)

### Construction

```cpp
FloatingTileWrapper::FloatingTileWrapper(ScriptContentComponent *content, 
    ScriptFloatingTile *floatingTile, int index):
    ScriptCreatedComponentWrapper(content, index)
{
    auto mc = const_cast<MainController*>(
        dynamic_cast<const Processor*>(content->getScriptProcessor())->getMainController());

    auto ft = new FloatingTile(mc, nullptr);
    ft->setIsFloatingTileOnInterface();
    component = ft;

    ft->setComponentID(floatingTile->getName().toString());
    ft->setName(floatingTile->name.toString());
    ft->setOpaque(false);
    ft->setContent(floatingTile->getContentData());
    ft->refreshRootLayout();

    for (const auto& c : floatingTile->getMouseListeners())
        mouseCallbacks.add(new AdditionalMouseCallback(floatingTile, component, c));

    updateLookAndFeel();
}
```

The wrapper creates an actual `FloatingTile` JUCE component with:
- `setIsFloatingTileOnInterface()` -- marks it as an interface-embedded tile (affects some behavior)
- `setOpaque(false)` -- always non-opaque
- `setContent(getContentData())` -- passes the JSON configuration to initialize the panel type

### Property Update

```cpp
void FloatingTileWrapper::updateComponent(int propertyIndex, var newValue)
{
    // ...
    switch (propertyIndex)
    {
    PROPERTY_CASE::ScriptComponent::itemColour: 
    PROPERTY_CASE::ScriptComponent::itemColour2:
    PROPERTY_CASE::ScriptFloatingTile::itemColour3:
    PROPERTY_CASE::ScriptComponent::bgColour: 
    PROPERTY_CASE::ScriptComponent::textColour: 
    PROPERTY_CASE::ScriptFloatingTile::Properties::Font:
    PROPERTY_CASE::ScriptFloatingTile::Properties::FontSize:
    PROPERTY_CASE::ScriptFloatingTile::Properties::Data:
    PROPERTY_CASE::ScriptFloatingTile::Properties::ContentType:
        ft->setContent(sft->getContentData());
        updateLookAndFeel();
        break;
    }

#if USE_BACKEND
    if (propertyIndex == ScriptFloatingTile::ContentType)
    {
        sft->fillScriptPropertiesWithFloatingTile(ft);
    }
#endif
}
```

**ALL visual properties trigger a full content reload.** Changing any colour, font, font size, data, or content type causes `ft->setContent(sft->getContentData())` -- the entire floating tile is rebuilt. This is because the FloatingTileContent system uses a monolithic JSON configuration.

In backend mode, changing ContentType also triggers `fillScriptPropertiesWithFloatingTile` which reads default colours from the new panel type back into the ScriptComponent properties.

### Value Update

```cpp
void FloatingTileWrapper::updateValue(var newValue)
{
    auto sft = dynamic_cast<ScriptFloatingTile*>(getScriptComponent());
    auto ft = dynamic_cast<FloatingTile*>(component.get());
    
    const bool updateAfterInit = (bool)sft->getScriptObjectProperty(
        ScriptFloatingTile::Properties::updateAfterInit);

    if (updateAfterInit)
    {
        ft->setContent(sft->getContentData());
        ft->refreshRootLayout();
    }
}
```

When `updateAfterInit` is true (default), calling `setValue()` causes a complete content reload. Setting it to false prevents value changes from triggering reloads -- useful if the floating tile content should remain static after initialization.

### LookAndFeel Propagation

The `updateLookAndFeel()` method recursively applies a ScriptedLookAndFeel to all child components of the FloatingTile, including ComplexDataUIBase editors. This means `setLocalLookAndFeel()` on a ScriptFloatingTile will style the internal panel content.

---

## ContentType Options (Frontend Panel Types)

The `getOptionsFor(ContentType)` method creates a `FloatingTileContent::Factory`, calls `registerFrontendPanelTypes()`, and returns its ID list. The available frontend panel types are:

| Panel ID | Class | Description |
|----------|-------|-------------|
| `"Empty"` | EmptyComponent | Empty placeholder |
| `"PresetBrowser"` | PresetBrowserPanel | Preset browser with bank/category/preset columns |
| `"AboutPagePanel"` | AboutPagePanel | About page with version info |
| `"Keyboard"` | MidiKeyboardPanel | Virtual MIDI keyboard |
| `"PerformanceLabel"` | PerformanceLabelPanel | CPU/voice count statistics |
| `"MidiOverlayPanel"` | MidiOverlayPanel | MIDI player transport overlay |
| `"ActivityLed"` | ActivityLedPanel | MIDI activity indicator |
| `"CustomSettings"` | CustomSettingsWindowPanel | Audio/MIDI device settings |
| `"MidiSources"` | MidiSourcePanel | MIDI input source selector |
| `"MidiChannelList"` | MidiChannelPanel | MIDI channel filter |
| `"TooltipPanel"` | TooltipPanel | Tooltip display area |
| `"MidiLearnPanel"` | MidiLearnPanel | MIDI learn assignment UI |
| `"FrontendMacroPanel"` | FrontendMacroPanel | Macro control panel |
| `"Plotter"` | PlotterPanel | Signal plotter |
| `"AudioAnalyser"` | AudioAnalyserComponent::Panel | FFT/oscilloscope/goniometer |
| `"Waveform"` | WaveformComponent::Panel | Wavetable preview |
| `"FilterDisplay"` | FilterGraph::Panel | Filter frequency response display |
| `"DraggableFilterPanel"` | FilterDragOverlay::Panel | Interactive filter control |
| `"WavetableWaterfall"` | WaterfallComponent::Panel | Waterfall/spectrogram display |
| `"MPEPanel"` | MPEPanel | MPE configuration panel |
| `"ModulationMatrix"` | ModulationMatrixPanel | Modulation matrix editor |
| `"ModulationMatrixController"` | ModulationMatrixControlPanel | Modulation matrix controller |
| `"AHDSRGraph"` | AhdsrEnvelope::Panel | AHDSR envelope graph |
| `"FlexAHDSRGraph"` | FlexAhdsrEnvelope::Panel | Flex AHDSR envelope graph |
| `"MarkdownPanel"` | MarkdownPreviewPanel | Markdown text renderer |
| `"MatrixPeakMeter"` | MatrixPeakMeter | Multi-channel peak meter |

Additionally, if `HI_ENABLE_EXTERNAL_CUSTOM_TILES` is defined, custom panel types registered via `registerExternalPanelTypes()` are also available.

---

## Font Options

```cpp
if (id == getIdFor(Font))
{
    StringArray sa;
    sa.add("Default");
    sa.add("Oxygen");
    sa.add("Source Code Pro");
    getScriptProcessor()->getMainController_()->fillWithCustomFonts(sa);
    sa.addArray(Font::findAllTypefaceNames());
    return sa;
}
```

Font choices include built-in fonts ("Default", "Oxygen", "Source Code Pro"), any custom fonts loaded via `Engine.loadFontAs()`, and all system fonts.

---

## fillScriptPropertiesWithFloatingTile (Backend Only)

```cpp
bool ScriptFloatingTile::fillScriptPropertiesWithFloatingTile(FloatingTile* ft)
{
    auto ftc = ft->getCurrentFloatingPanel();

    setScriptObjectProperty(bgColour, (int64)ftc->getDefaultPanelColour(
        FloatingTileContent::PanelColourId::bgColour).getARGB(), sendNotification);
    setScriptObjectProperty(itemColour, (int64)ftc->getDefaultPanelColour(
        FloatingTileContent::PanelColourId::itemColour1).getARGB(), sendNotification);
    setScriptObjectProperty(itemColour2, (int64)ftc->getDefaultPanelColour(
        FloatingTileContent::PanelColourId::itemColour2).getARGB(), sendNotification);
    setScriptObjectProperty(textColour, (int64)ftc->getDefaultPanelColour(
        FloatingTileContent::PanelColourId::textColour).getARGB(), sendNotification);

    auto data = ftc->toDynamicObject();

    if (auto obj = data.getDynamicObject())
    {
        // Remove properties that are managed by the ScriptComponent properties
        obj->removeProperty(getDefaultablePropertyId(ColourData));
        obj->removeProperty(getDefaultablePropertyId(StyleData));
        obj->removeProperty(getDefaultablePropertyId(LayoutData));
        obj->removeProperty(getDefaultablePropertyId(Font));
        obj->removeProperty(getDefaultablePropertyId(FontSize));
        obj->removeProperty(getDefaultablePropertyId(Type));

        setScriptObjectProperty(Data, JSON::toString(data, false, DOUBLE_TO_STRING_DIGITS), sendNotification);
    }

    return true;
}
```

This reads the current floating tile panel's default colours and data back into the ScriptComponent properties. Called in backend mode when the ContentType is changed -- it auto-populates the colour properties and Data JSON string with the panel's defaults. The standard FloatingTileContent properties (ColourData, StyleData, LayoutData, Font, FontSize, Type) are stripped from the Data JSON since they are managed by the dedicated ScriptComponent properties.

---

## Export/Restore (ValueTree Serialization)

```cpp
ValueTree ScriptFloatingTile::exportAsValueTree() const
{
    auto v = ScriptComponent::exportAsValueTree();
    return v;
}

void ScriptFloatingTile::restoreFromValueTree(const ValueTree &v)
{
    ScriptComponent::restoreFromValueTree(v);
}
```

No custom serialization -- delegates entirely to ScriptComponent base. The jsonData is reconstructed from the ContentType, Data, Font, FontSize, and colour properties during property restoration via `setScriptObjectPropertyWithChangeMessage`.

---

## JSON Data Structure Expected by setContentData

The JSON object passed to `setContentData()` should match the FloatingTileContent configuration format:

```json
{
    "Type": "PresetBrowser",
    "Font": "Oxygen Bold",
    "FontSize": 16.0,
    "ColourData": {
        "bgColour": "0xFF222222",
        "textColour": "0xFFFFFFFF",
        "itemColour1": "0xFF444444",
        "itemColour2": "0xFF666666",
        "itemColour3": "0xFF888888"
    },
    // ... panel-specific properties
}
```

Note: The `"Type"` property is mandatory -- it determines which panel class gets instantiated. The colour property names in ColourData use `"itemColour1"` (not `"itemColour"`) -- hence the rename in `setScriptObjectPropertyWithChangeMessage`.

---

## Relationship to the HISE FloatingTile System

### How it Works

1. `Content.addFloatingTile("name", x, y)` creates a ScriptFloatingTile
2. The ScriptFloatingTile stores configuration as properties (ContentType, colours, font, data) and an internal jsonData object
3. The FloatingTileWrapper creates an actual `FloatingTile` JUCE component
4. `FloatingTile::setContent(jsonData)` is called, which:
   - Creates a `FloatingTileContent::Factory`
   - Uses the factory to create the appropriate panel class from the "Type" property
   - Calls `fromDynamicObject()` on the panel to configure it

### Interface vs. Backend FloatingTiles

The `setIsFloatingTileOnInterface()` call marks the FloatingTile as being on the script interface rather than in the backend IDE. This flag (`interfaceFloatingTile = true`) affects some panel behaviors.

### Panel Type Resolution

Only **frontend panel types** are available in ScriptFloatingTile. The factory is created fresh and only `registerFrontendPanelTypes()` is called -- NOT `registerAllPanelTypes()` or `registerBackendPanelTypes()`. This means backend-only panels (ScriptEditor, Console, SampleEditor, etc.) cannot be embedded as ScriptFloatingTile content.

---

## Threading and Lifecycle

- **Creation:** onInit only (via Content.addFloatingTile)
- **setContentData:** Can be called after onInit. Triggers property change notifications that cause the wrapper to rebuild the floating tile content.
- **setValue/getValue:** Thread-safe in the sense that they are trivial assignments (no locks), but the actual content reload happens on the message thread via the wrapper's updateValue.
- **updateAfterInit:** Controls whether setValue triggers content reload. Set to false if you want to configure the content once and never update it dynamically.

---

## Preprocessor Guards

- `#if USE_BACKEND` -- `fillScriptPropertiesWithFloatingTile` is only called in backend mode to sync panel defaults back to properties
- `#if HI_ENABLE_EXTERNAL_CUSTOM_TILES` -- Allows third-party panel types to be registered

---

## No Constants

No `addConstant()` calls in the constructor. The class has no script-accessible constants.
