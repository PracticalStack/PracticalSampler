# ScriptAudioWaveform -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/explorations/ScriptComponent_base.md` -- Base ScriptComponent class exploration
- `enrichment/resources/survey/class_survey.md` -- Enrichment prerequisites
- `enrichment/resources/survey/class_survey_data.json` -- ScriptAudioWaveform entry (createdBy, seeAlso)
- No ComplexDataScriptComponent base exploration exists -- explored inline

## Source Files

- **Header:** `hi_scripting/scripting/api/ScriptingApiContent.h` (lines 1593-1666)
- **Implementation:** `hi_scripting/scripting/api/ScriptingApiContent.cpp` (lines 3359-4028)
- **Wrapper:** `hi_scripting/scripting/api/ScriptComponentWrappers.h` (lines 952-975) and `ScriptComponentWrappers.cpp` (lines 2630-2906)
- **Base class:** `ComplexDataScriptComponent` in `ScriptingApiContent.h` (lines 1358-1420) and `.cpp` (lines 3157-3356)
- **JUCE rendering:** `hi_tools/hi_standalone_components/SampleDisplayComponent.h` (AudioDisplayComponent, HiseAudioThumbnail, MultiChannelAudioBufferDisplay, MultiChannelAudioBuffer)
- **Sampler rendering:** `hi_core/hi_components/audio_components/SampleComponents.h` (SamplerSoundWaveform)

---

## Inheritance Chain

```
ScriptComponent (ConstScriptingObject, RestorableObject, AssignableObject, SafeChangeBroadcaster)
  |
  +-- ComplexDataScriptComponent (also ExternalDataHolder, ComplexDataUIUpdaterBase::EventListener)
        |
        +-- ScriptAudioWaveform
```

ScriptAudioWaveform inherits 35 methods from ScriptComponent (documented in ScriptComponent_base.md) and adds 6 own methods.

---

## ComplexDataScriptComponent Layer

This intermediate class connects UI components to HISE's complex data system (`ExternalData` types). It is shared by `ScriptTable`, `ScriptSliderPack`, and `ScriptAudioWaveform`.

### Constructor

```cpp
ComplexDataScriptComponent(ProcessorWithScriptingContent* base, Identifier name, snex::ExternalData::DataType type_):
    ScriptComponent(base, name),
    type(type_)
{
    ownedObject = snex::ExternalData::create(type);
    ownedObject->setGlobalUIUpdater(base->getMainController_()->getGlobalUIUpdater());
    ownedObject->setUndoManager(base->getMainController_()->getControlUndoManager());
}
```

Key: Each complex data component creates and owns a `ComplexDataUIBase` object of its type. For `ScriptAudioWaveform`, this creates a `MultiChannelAudioBuffer`.

### ExternalDataHolder Implementation

The class implements `ExternalDataHolder`, exposing the complex data to the HISE module system. It provides `getTable()`, `getSliderPack()`, `getAudioFile()`, etc., returning `nullptr` for non-matching types and delegating to `getUsedData()` for matching types.

### Data Source Resolution (getUsedData)

```cpp
ComplexDataUIBase* getUsedData(snex::ExternalData::DataType requiredType) {
    if (type != requiredType) return nullptr;
    if (auto eh = getExternalHolder()) {
        auto externalIndex = (int)getScriptObjectProperty(getIndexPropertyId());
        cachedObjectReference = eh->getComplexBaseType(type, externalIndex);
    } else
        cachedObjectReference = ownedObject.get();
    return cachedObjectReference;
}
```

Data source priority:
1. `otherHolder` (set via `referToData`) -- another ExternalDataHolder
2. Connected processor (via `processorId` property) -- the processor's ExternalDataHolder
3. Own internal `ownedObject` -- the default

### referToDataBase (shared implementation)

```cpp
void referToDataBase(var newData) {
    if (auto td = dynamic_cast<ScriptComplexDataReferenceBase*>(newData.getObject())) {
        // Connect to ScriptAudioFile, ScriptSliderPackData, or ScriptTableData
        otherHolder = td->getHolder();
        setScriptObjectPropertyWithChangeMessage(getIdFor(getIndexPropertyId()), td->getIndex(), sendNotification);
        updateCachedObjectReference();
    } else if (auto cd = dynamic_cast<ComplexDataScriptComponent*>(newData.getObject())) {
        // Connect to another complex data UI component
        otherHolder = cd;
        updateCachedObjectReference();
    } else if ((newData.isInt() || newData.isInt64()) && (int)newData == -1) {
        // Reset to own internal data
        otherHolder = nullptr;
        updateCachedObjectReference();
    }
}
```

Accepts three argument types:
- A `ScriptComplexDataReferenceBase` (e.g., `ScriptAudioFile` from `Engine.createAudioFile()`)
- Another `ComplexDataScriptComponent` (e.g., another ScriptAudioWaveform)
- The integer `-1` to reset to internal data

### registerComplexDataObjectAtParent (shared implementation)

```cpp
var registerComplexDataObjectAtParent(int index) {
    if (auto d = dynamic_cast<ProcessorWithDynamicExternalData*>(getScriptProcessor())) {
        otherHolder = d;
        d->registerExternalObject(type, index, ownedObject.get());
        setScriptObjectProperty(getIndexPropertyId(), index, sendNotification);
        updateCachedObjectReference();
        // Returns a new ScriptAudioFile handle for AudioFile type
        switch (type) {
        case ExternalData::DataType::AudioFile:
            return new ScriptingObjects::ScriptAudioFile(getScriptProcessor(), index);
        // ... other types
        }
    }
    return var();
}
```

This registers the component's internal data object with the parent script processor, making it accessible from scriptnode or external code. Returns a `ScriptAudioFile` handle for waveform components.

### Deactivated Properties (ComplexData level)

```cpp
void handleDefaultDeactivatedProperties() {
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(macroControl));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(parameterId));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(linkedTo));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(isMetaParameter));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(isPluginParameter));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(pluginParameterName));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(automationId));
}
```

### Persistence (exportAsValueTree / restoreFromValueTree)

Complex data state is serialized via `toBase64String()` and restored via `fromBase64String()`, stored in a `"data"` property on the ValueTree.

### updateCachedObjectReference

Manages event listener registration: removes from old, adds to new, and notifies the `sourceWatcher` about the new source. This is called whenever `processorId` or `sampleIndex` changes.

---

## ScriptAudioWaveform Class Declaration

### Properties Enum

```cpp
enum Properties {
    itemColour3 = ScriptComponent::Properties::numProperties,
    opaque,
    showLines,
    showFileName,
    sampleIndex,
    enableRange,
    loadWithLeftClick,
    numProperties
};
```

7 properties unique to ScriptAudioWaveform, starting after ScriptComponent's `numProperties`.

### getIndexPropertyId

```cpp
int getIndexPropertyId() const override { return sampleIndex; }
```

The `sampleIndex` property serves as the index into the connected processor's external data slots.

### Private Members

```cpp
MultiChannelAudioBuffer* getCachedAudioFile() { return static_cast<MultiChannelAudioBuffer*>(getCachedDataObject()); };
const MultiChannelAudioBuffer* getCachedAudioFile() const { return static_cast<const MultiChannelAudioBuffer*>(getCachedDataObject()); };
```

Convenience casts to the correct complex data type.

---

## Constructor

```cpp
ScriptAudioWaveform(ProcessorWithScriptingContent *base, Content*, Identifier waveformName, int x, int y, int, int) :
    ComplexDataScriptComponent(base, waveformName, snex::ExternalData::DataType::AudioFile)
{
    // Properties
    ADD_SCRIPT_PROPERTY(i01, "itemColour3"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
    ADD_SCRIPT_PROPERTY(i02, "opaque");      ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i03, "showLines");   ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i04, "showFileName");ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i05, "sampleIndex");
    ADD_SCRIPT_PROPERTY(i06, "enableRange"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i07, "loadWithLeftClick"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

    // Defaults
    setDefaultValue(ScriptComponent::Properties::width, 200);
    setDefaultValue(ScriptComponent::Properties::height, 100);

    setDefaultValue(Properties::itemColour3, 0x22FFFFFF);
    setDefaultValue(ScriptComponent::Properties::bgColour, (int64)0xFF555555);
    setDefaultValue(ScriptComponent::Properties::itemColour2, (int64)0xffcccccc);
    setDefaultValue(ScriptComponent::Properties::itemColour, (int64)0xa2181818);

    setDefaultValue(Properties::opaque, true);
    setDefaultValue(Properties::showLines, false);
    setDefaultValue(Properties::showFileName, true);
    setDefaultValue(Properties::sampleIndex, 0);
    setDefaultValue(Properties::enableRange, true);
    setDefaultValue(Properties::loadWithLeftClick, false);

    handleDefaultDeactivatedProperties();
    updateCachedObjectReference();

    // API methods -- all untyped (ADD_API_METHOD_N, no forced types)
    ADD_API_METHOD_1(referToData);
    ADD_API_METHOD_0(getRangeStart);
    ADD_API_METHOD_0(getRangeEnd);
    ADD_API_METHOD_1(setDefaultFolder);
    ADD_API_METHOD_1(registerAtParent);
    ADD_API_METHOD_1(setPlaybackPosition);
}
```

### Method Registration (Wrapper struct)

```cpp
struct Wrapper {
    API_VOID_METHOD_WRAPPER_1(ScriptAudioWaveform, referToData);
    API_METHOD_WRAPPER_0(ScriptAudioWaveform, getRangeStart);
    API_METHOD_WRAPPER_0(ScriptAudioWaveform, getRangeEnd);
    API_METHOD_WRAPPER_1(ScriptAudioWaveform, registerAtParent);
    API_VOID_METHOD_WRAPPER_1(ScriptAudioWaveform, setDefaultFolder);
    API_VOID_METHOD_WRAPPER_1(ScriptAudioWaveform, setPlaybackPosition);
};
```

All use `ADD_API_METHOD_N` (untyped). No `ADD_TYPED_API_METHOD_N` usage -- zero forced parameter types.

---

## Deactivated Properties (ScriptAudioWaveform level)

```cpp
void handleDefaultDeactivatedProperties() {
    ComplexDataScriptComponent::handleDefaultDeactivatedProperties();
    // Additional deactivations on top of ComplexData's:
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(text));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(min));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(max));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(defaultValue));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(macroControl));
}
```

Combined deactivated properties (from ComplexData + ScriptAudioWaveform):
- `text`, `min`, `max`, `defaultValue`, `macroControl`
- `parameterId`, `linkedTo`, `isMetaParameter`, `isPluginParameter`, `pluginParameterName`, `automationId`

---

## Method Implementations

### referToData(var audioData)

```cpp
void referToData(var audioData) {
    referToDataBase(audioData);
}
```

Pure delegation to `ComplexDataScriptComponent::referToDataBase()`. See detailed analysis above. Accepts a `ScriptAudioFile` object, another `ScriptAudioWaveform`, or `-1`.

### getRangeStart() / getRangeEnd()

```cpp
int getRangeStart() {
    if (auto af = getCachedAudioFile())
        return af->getCurrentRange().getStart();
    return 0;
}

int getRangeEnd() {
    if (auto af = getCachedAudioFile())
        return af->getCurrentRange().getEnd();
    return 0;
}
```

Returns the current sample range (in samples) of the audio data. The range is set by user interaction with the draggable SampleArea or programmatically.

### registerAtParent(int pIndex)

```cpp
var registerAtParent(int pIndex) {
    return registerComplexDataObjectAtParent(pIndex);
}
```

Delegates to base. Returns a `ScriptAudioFile` handle that can be used to programmatically control the audio data.

### setDefaultFolder(var newDefaultFolder)

```cpp
void setDefaultFolder(var newDefaultFolder) {
    if (auto af = getCachedAudioFile()) {
        if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(newDefaultFolder.getObject()))
            af->getProvider()->setRootDirectory(sf->f);
        else
            reportScriptError("newDefaultFolder must be a File object");
    }
}
```

Requires a `ScriptFile` object (from `FileSystem.getFolder()` etc.). Sets the root directory on the `MultiChannelAudioBuffer::DataProvider`, which is used when the file browser opens.

### setPlaybackPosition(double normalisedPosition)

```cpp
void setPlaybackPosition(double normalisedPosition) {
    if (auto af = getCachedAudioFile()) {
        auto sampleIndex = roundToInt((double)af->getCurrentRange().getLength() * normalisedPosition);
        af->getUpdater().sendDisplayChangeMessage(sampleIndex, sendNotificationAsync, true);
    }
}
```

Converts a 0.0-1.0 normalized position to a sample index relative to the current range length, then sends an async display change message through the `ComplexDataUIUpdater`. This moves the playback cursor displayed on the waveform.

---

## Persistence (ScriptAudioWaveform overrides)

### exportAsValueTree

```cpp
ValueTree exportAsValueTree() const {
    ValueTree v = ComplexDataScriptComponent::exportAsValueTree();
    if (auto af = getCachedAudioFile()) {
        auto sr = af->getCurrentRange();
        v.setProperty("rangeStart", sr.getStart(), nullptr);
        v.setProperty("rangeEnd", sr.getEnd(), nullptr);
    }
    return v;
}
```

Adds `rangeStart` and `rangeEnd` to the serialized state on top of the base64-encoded audio data.

### restoreFromValueTree

```cpp
void restoreFromValueTree(const ValueTree &v) {
    ComplexDataScriptComponent::restoreFromValueTree(v);
    if (auto af = getCachedAudioFile()) {
        // Legacy compatibility: old HISE versions stored as "fileName" instead of "data"
        if (v.hasProperty("fileName") && !v.hasProperty("data"))
            af->fromBase64String(v.getProperty("fileName", "").toString());
        Range<int> range(v.getProperty("rangeStart", 0), v.getProperty("rangeEnd", 0));
        af->setRange(range);
    }
}
```

Handles legacy `fileName` property for backwards compatibility with older HISE versions.

---

## getOptionsFor (processorId dropdown)

```cpp
StringArray getOptionsFor(const Identifier &id) {
    if (id == getIdFor(processorId)) {
        auto list = ComplexDataScriptComponent::getOptionsFor(id);
        // ComplexData base returns all processors with AudioFile data
        auto os = ProcessorHelpers::findParentProcessor(dynamic_cast<Processor*>(getScriptProcessor()), true);
        auto samplers = ProcessorHelpers::getAllIdsForType<ModulatorSampler>(os);
        list.addArray(samplers);
        return list;
    }
    return ScriptComponent::getOptionsFor(id);
}
```

The `processorId` property dropdown shows:
1. All processors that have AudioFile external data (from ComplexData base)
2. All ModulatorSampler processors (added by ScriptAudioWaveform)

This is significant: ScriptAudioWaveform can connect to either AudioFile-holding processors OR ModulatorSamplers.

---

## getSampler()

```cpp
ModulatorSampler* getSampler() {
    return dynamic_cast<ModulatorSampler*>(getConnectedProcessor());
}
```

Returns the connected processor as a `ModulatorSampler` if it is one, `nullptr` otherwise. This determines which JUCE component type is created for rendering.

---

## resetValueToDefault()

```cpp
void resetValueToDefault() {
    if (auto af = getCachedAudioFile())
        af->fromBase64String({});
}
```

Clears the audio file by passing an empty string to `fromBase64String`.

---

## JUCE Component Wrapper (AudioWaveformWrapper)

### Dual-mode Component Creation

The wrapper creates one of two different JUCE components based on whether the connected processor is a `ModulatorSampler`:

```cpp
AudioWaveformWrapper(ScriptContentComponent *content, ScriptAudioWaveform *form, int index) :
    ScriptCreatedComponentWrapper(content, index)
{
    if (auto s = form->getSampler()) {
        // SAMPLER MODE: Show sampler sound waveform
        SamplerSoundWaveform* ssw = new SamplerSoundWaveform(s);
        ssw->getSampleArea(SamplerSoundWaveform::PlayArea)->setAreaEnabled(true);
        ssw->setIsOnInterface(true);
        component = ssw;
        samplerListener = new SamplerListener(s, ssw);
    } else {
        // AUDIO FILE MODE: Show multi-channel audio buffer
        MultiChannelAudioBufferDisplay* asb = new MultiChannelAudioBufferDisplay();
        component = asb;
    }
    form->getSourceWatcher().addSourceListener(this);
    initAllProperties();

    // LAF setup for thumbnails
    if (auto adc = dynamic_cast<AudioDisplayComponent*>(component.get())) {
        if (auto slaf = dynamic_cast<CSSLaf*>(localLookAndFeel.get()))
            Component::callRecursive<ResizableEdgeComponent>(adc, [slaf](ResizableEdgeComponent* edge) {
                edge->setLookAndFeel(slaf);
                return false;
            });
        if (auto l = dynamic_cast<HiseAudioThumbnail::LookAndFeelMethods*>(localLookAndFeel.get()))
            adc->getThumbnail()->setLookAndFeel(localLookAndFeel);
        else if (auto s = dynamic_cast<HiseAudioThumbnail::LookAndFeelMethods*>(slaf))
            adc->getThumbnail()->setLookAndFeel(slaf);
    }
}
```

### Two Rendering Modes

| Mode | JUCE Component | When | Features |
|------|---------------|------|----------|
| **Sampler** | `SamplerSoundWaveform` | `processorId` points to a `ModulatorSampler` | Shows sample from sampler, play/loop/crossfade areas, auto-updates on voice start |
| **AudioFile** | `MultiChannelAudioBufferDisplay` | `processorId` points to any AudioFile-holding processor, or no processor | File browser, drag-and-drop, range selection |

Both inherit from `AudioDisplayComponent`, which provides the common `HiseAudioThumbnail`, `SampleArea` system, and playback position.

### SamplerListener (inner class)

When in Sampler mode, a `SamplerListener` handles:

- **Voice tracking:** `otherChange()` -- when `sampleIndex == -1` (default), auto-displays the last started voice's sound
- **Sample map changes:** `sampleMapWasChanged()`, `sampleAmountChanged()`, `sampleMapCleared()` -- refreshes display
- **Sample property changes:** `samplePropertyWasChanged()` -- updates ranges when audio properties change
- **Range changes:** `rangeChanged()` -- when user drags range edges, writes back to `SampleStart`/`SampleEnd`/`LoopStart`/`LoopEnd` sample properties
- **Manual sample index:** When `sampleIndex != -1`, displays a specific sound by index

### Property Handling (updateComponent)

```cpp
void updateComponent(int propertyIndex, var newValue) {
    if (auto adc = dynamic_cast<AudioDisplayComponent*>(component.get())) {
        switch (propertyIndex) {
            // Common to both modes:
            PROPERTY_CASE enabled: adc->getSampleArea(0)->setEnabled((bool)newValue); break;
            PROPERTY_CASE opaque: adc->setOpaque((bool)newValue); break;
            PROPERTY_CASE processorId:
            PROPERTY_CASE sampleIndex: updateComplexDataConnection(); break;
            PROPERTY_CASE itemColour3:
            PROPERTY_CASE itemColour:
            PROPERTY_CASE itemColour2:
            PROPERTY_CASE bgColour:
            PROPERTY_CASE textColour: updateColours(adc); break;
            PROPERTY_CASE tooltip: adc->setTooltip(GET_SCRIPT_PROPERTY(tooltip)); break;
            PROPERTY_CASE showLines: adc->getThumbnail()->setDrawHorizontalLines((bool)newValue); break;
            PROPERTY_CASE enableRange: adc->getSampleArea(0)->setAreaEnabled(newValue); break;
        }
        // AudioFile mode only:
        if (auto asb = dynamic_cast<MultiChannelAudioBufferDisplay*>(component.get())) {
            switch (propertyIndex) {
                PROPERTY_CASE showFileName: asb->setShowFileName((bool)newValue); break;
                PROPERTY_CASE loadWithLeftClick: asb->setLoadWithLeftClick((bool)newValue); break;
            }
        }
    }
}
```

### Colour Mapping

```cpp
void updateColours(AudioDisplayComponent* asb) {
    asb->setColour(AudioDisplayComponent::bgColour, GET_OBJECT_COLOUR(bgColour));
    auto tn = asb->getThumbnail();
    tn->setColour(AudioDisplayComponent::outlineColour, GET_OBJECT_COLOUR(itemColour));
    tn->setColour(AudioDisplayComponent::fillColour, GET_OBJECT_COLOUR(itemColour2));
    tn->setColour(AudioDisplayComponent::textColour, GET_OBJECT_COLOUR(textColour));
}
```

| Script Property | AudioDisplayComponent ColourId | Purpose |
|----------------|-------------------------------|---------|
| `bgColour` | `bgColour` | Component background |
| `itemColour` | `outlineColour` | Waveform outline |
| `itemColour2` | `fillColour` | Waveform fill |
| `textColour` | `textColour` | Text overlay (filename, etc.) |
| `itemColour3` | (triggers updateColours) | Additional waveform colour (default: 0x22FFFFFF, semi-transparent white) |

Note: `itemColour3` triggers `updateColours()` but is not explicitly mapped to a ColourId in the colour update function. Its actual effect depends on the LAF and thumbnail rendering.

### updateComplexDataConnection (sampler mode special handling)

```cpp
void updateComplexDataConnection() {
    if (auto s = dynamic_cast<ModulatorSampler*>(getScriptComponent()->getConnectedProcessor())) {
        if (auto ssw = dynamic_cast<SamplerSoundWaveform*>(component.get())) {
            auto index = (int)getScriptComponent()->getScriptObjectProperty(sampleIndex);
            if (samplerListener != nullptr) {
                samplerListener->setActive(index == -1);
                samplerListener->displayedIndex = index;
            }
            if (index != -1 && lastIndex != index) {
                ssw->setSoundToDisplay(dynamic_cast<ModulatorSamplerSound*>(s->getSound(index)), 0);
                lastIndex = index;
            }
        }
    } else {
        ScriptCreatedComponentWrapper::updateComplexDataConnection(); // base implementation for AudioFile mode
    }
}
```

When `sampleIndex == -1`: SamplerListener is active and auto-displays the last started voice's sound.
When `sampleIndex >= 0`: SamplerListener is deactivated and a specific sound is displayed by index.

---

## AudioDisplayComponent Hierarchy

```
HiseAudioThumbnail (renders the actual waveform path)
  |
AudioDisplayComponent (base: SampleArea management, playback position)
  |
  +-- SamplerSoundWaveform (sampler mode: shows ModulatorSamplerSound)
  |     - Has PlayArea, SampleStartArea, LoopArea, LoopCrossfadeArea
  |     - Timer-based playback position updates
  |     - Listens for voice starts to show current sound
  |
  +-- MultiChannelAudioBufferDisplay (audio file mode)
        - Has only PlayArea
        - FileDragAndDropTarget, DragAndDropTarget support
        - File browser via right-click or left-click (configurable)
        - Shows filename overlay
```

### AudioDisplayComponent::SampleArea

Each `SampleArea` is a draggable range rectangle with:
- Left/right edge resize handles (`ResizableEdgeComponent`)
- `getSampleRange()` / `setSampleRange()` -- range in samples
- `setAreaEnabled()` -- toggle draggability
- `setAllowedPixelRanges()` -- constrain edge positions
- Change messages sent on mouse-up

### HiseAudioThumbnail

The core waveform renderer:

#### Display Modes
```cpp
enum class DisplayMode {
    SymmetricArea,    // Default -- fills waveform symmetrically
    DownsampledCurve, // Draws curve from downsampled data
    numDisplayModes
};
```

#### RenderOptions
```cpp
struct RenderOptions {
    DisplayMode displayMode = DisplayMode::SymmetricArea;
    float manualDownSampleFactor = -1.0f;
    int multithreadThreshold = 44100;
    bool drawHorizontalLines = false;
    bool scaleVertically = false;
    float displayGain = 1.0f;
    bool useRectList = false;
    int forceSymmetry = 0;
    bool dynamicOptions = false;
};
```

#### LookAndFeelMethods

The LAF methods on `HiseAudioThumbnail::LookAndFeelMethods`:

```cpp
virtual void drawHiseThumbnailBackground(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area);
virtual void drawHiseThumbnailPath(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path);
virtual void drawHiseThumbnailRectList(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const RectangleListType& rectList);
virtual void drawTextOverlay(Graphics& g, HiseAudioThumbnail& th, const String& text, Rectangle<float> area);
virtual void drawThumbnailRange(Graphics& g, HiseAudioThumbnail& te, Rectangle<float> area, int areaIndex, Colour c, bool areaEnabled);
virtual void drawThumbnailRuler(Graphics& g, HiseAudioThumbnail& te, int xPosition);
virtual RenderOptions getThumbnailRenderOptions(HiseAudioThumbnail& te, const RenderOptions& defaultRenderOptions);
```

These map to the LAF functions exposed via `ScriptLookAndFeel`:
- `drawThumbnailBackground`
- `drawThumbnailPath`
- `drawThumbnailText`
- `drawThumbnailRange`
- `drawThumbnailRuler`
- `getThumbnailRenderOptions`
- `drawMidiDropper` (for MIDI file dropping, separate)

---

## MultiChannelAudioBuffer -- The Data Object

This is the complex data type that backs `ScriptAudioWaveform` in AudioFile mode.

### Key Features
- Multi-channel audio buffer with original + data copies
- Sample range selection (`getCurrentRange()`, `setRange()`)
- Loop range (`getLoopRange()`, `setLoopRange()`)
- Base64 serialization
- `DataProvider` -- pluggable file loading and directory management
- `XYZProvider` -- sample map-style multi-file mapping
- Listener system with synchronous (data load) and asynchronous (sample index change) notifications

### DataProvider

```cpp
struct DataProvider: public ReferenceCountedObject {
    virtual SampleReference::Ptr loadFile(const String& referenceString) = 0;
    virtual File parseFileReference(const String& b64) const = 0;
    virtual File getRootDirectory();
    virtual void setRootDirectory(const File& rootDirectory);
};
```

`setDefaultFolder()` calls `af->getProvider()->setRootDirectory(sf->f)`, which sets the default folder for the file browser.

---

## Properties Summary

### ScriptAudioWaveform-specific Properties

| Property | Default | Type | Description |
|----------|---------|------|-------------|
| `itemColour3` | `0x22FFFFFF` | Colour | Additional waveform colour |
| `opaque` | `true` | Toggle | Whether component is opaque |
| `showLines` | `false` | Toggle | Draw horizontal lines on waveform |
| `showFileName` | `true` | Toggle | Show filename overlay (AudioFile mode only) |
| `sampleIndex` | `0` | Number | Index into processor's audio file slots; -1 for auto-display in sampler mode |
| `enableRange` | `true` | Toggle | Enable draggable sample range selection |
| `loadWithLeftClick` | `false` | Toggle | Open file browser on left click (AudioFile mode only) |

### Inherited Property Defaults (overridden from base)

| Property | ScriptAudioWaveform Default | Base Default |
|----------|---------------------------|-------------|
| `width` | 200 | (set by creation) |
| `height` | 100 | (set by creation) |
| `bgColour` | `0xFF555555` | `0x55FFFFFF` |
| `itemColour` | `0xa2181818` | `0x66333333` |
| `itemColour2` | `0xffcccccc` | `0xFB111111` |

---

## Factory Method

Created via `Content.addAudioWaveform(name, x, y)`:

```cpp
ScriptAudioWaveform* Content::addAudioWaveform(Identifier audioWaveformName, int x, int y) {
    return addComponent<ScriptAudioWaveform>(audioWaveformName, x, y);
}
```

Or in HiseScript: `const var waveform = Content.addAudioWaveform("Waveform1", 0, 0);`

---

## No Constants

The constructor has no `addConstant()` calls. ScriptAudioWaveform defines no scripting constants.

---

## No Preprocessor Guards

No `#if USE_BACKEND` or other preprocessor guards affect the API methods or properties.

---

## Threading Notes

- `setPlaybackPosition()` sends async display change messages -- safe from any thread
- `getRangeStart()`/`getRangeEnd()` read from `MultiChannelAudioBuffer::getCurrentRange()` -- lightweight reads
- `referToData()` and `registerAtParent()` modify the data source connection -- should be called from `onInit`
- `setDefaultFolder()` modifies the DataProvider's root directory -- should be called from `onInit`
