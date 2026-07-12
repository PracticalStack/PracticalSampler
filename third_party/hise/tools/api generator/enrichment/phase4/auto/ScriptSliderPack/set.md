Sets a component property to the given value. During `onInit`, changes are applied without UI notification; outside `onInit`, the UI updates automatically.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `text`, `tooltip` | Display text and hover tooltip |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback`, *`CallbackOnMouseUpOnly`* | Preset persistence, undo, callback deferral, and callback timing |
| `processorId`, *`SliderPackIndex`* | Complex data source: the connected processor and the slider pack slot to use from that processor or external data holder |
| `min`, `max`, `defaultValue`, *`StepSize`*, *`SliderAmount`* | Slider range, default values, step size, and number of sliders |
| *`FlashActive`*, *`ShowValueOverlay`* | Visual feedback for active sliders and value display |
| *`StepSequencerMode`* | Enables the step-sequencer interaction mode |
