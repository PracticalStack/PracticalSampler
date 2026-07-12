ScriptSliderPack::set(String propertyName, NotUndefined value) -> undefined

Thread safety: UNSAFE
Sets a component property to the given value. Reports a script error if the
property does not exist. During onInit, changes are applied without UI notification;
outside onInit, sends change notifications to update the UI.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `text`, `tooltip` | Display text and hover tooltip |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback`, *`mouseUpCallback`* | Preset persistence, undo, callback deferral, and callback timing |
| `processorId`, *`SliderPackIndex`* | Complex data source: the connected processor and the slider pack slot to use from that processor or external data holder |
| `min`, `max`, `defaultValue`, *`stepSize`*, *`sliderAmount`* | Slider range, default values, step size, and number of sliders |
| *`flashActive`*, *`showValueOverlay`* | Visual feedback for active sliders and value display |
| *`stepSequencerMode`* | Enables the step-sequencer interaction mode |

Dispatch/mechanics:
  sliderAmount/defaultValue/min/max/stepSize/flashActive/showValueOverlay map into SliderPackData setters.
  Wrapper updateComponent then applies UI-facing behavior on the JUCE SliderPack widget.

Pair with:
  get -- read back the same property
  getAllProperties -- discover active property IDs

Source:
  ScriptingApiContent.cpp:3513  ScriptSliderPack property mapping in setScriptObjectPropertyWithChangeMessage()
