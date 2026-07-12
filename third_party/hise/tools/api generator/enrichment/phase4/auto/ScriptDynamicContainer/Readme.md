<!-- Diagram triage:
  - No diagram specifications found in source data.
-->

# ScriptDynamicContainer

ScriptDynamicContainer is a UI container created with `Content.addDynamicContainer(name, x, y)` that builds child components from JSON at runtime. Use it when you want to create many controls from data instead of adding them one by one in `onInit`.

You pass a JSON array (or single object) to `setData()`, and the container creates the child components for you. It returns `ContainerChild` references so you can still access the generated controls afterwards.

The container supports these dynamic child component types:

- Button - toggle or momentary
- Slider - knob/slider with range and mode
- ComboBox - dropdown selector
- Label - text input/display
- Panel - custom drawable panel
- FloatingTile - embedded HISE floating tile widget
- DragContainer - drag-reorderable container
- Viewport - flexbox layout container
- TextBox - multi-line text display
- TableEditor - table curve editor
- SliderPack - multi-slider array
- AudioFile - audio waveform display

The container separates setup data from live values. In practice this means:

- the JSON describes what should be created and how it should look
- the internal value model stores the current user state of those controls
- a single value callback can react to changes from any generated child

It also integrates with user presets, so child values are restored automatically without extra bookkeeping.

```js
const var dc = Content.addDynamicContainer("MyContainer", 0, 0);
dc.setPosition(0, 0, 500, 300);

const var root = dc.setData([
    {"id": "Knob1", "type": "Slider", "text": "Volume"},
    {"id": "Btn1",  "type": "Button", "text": "Bypass"}
]);

dc.setValueCallback(function(id, value)
{
    Console.print(id + " = " + value);
});
```

> [!Tip:Data model replaces preset and macro properties] The container deactivates the usual preset, parameter, and macro properties because value handling goes through the internal data model instead. Calling `setData()` rebuilds the child tree and invalidates older `ContainerChild` references, so always fetch new references after rebuilding. Legacy type names (`ScriptButton`, `ScriptSlider`, etc.) are accepted and converted automatically.

## Common Mistakes

- **Call setData before setValueCallback**
  **Wrong:** Calling `setValueCallback()` before `setData()`
  **Right:** Call `setData()` first, then register the value callback.
  *The value callback listens to the data model's internal value store, which does not exist until `setData()` creates it. Calling `setValueCallback()` first silently does nothing.*

- **ContainerChild invalid after setData**
  **Wrong:** Storing a ContainerChild reference and using it after a new `setData()` call
  **Right:** Re-obtain references after each `setData()` call, or check `ref.isValid()` before use.
  *Each `setData()` call rebuilds the component tree and invalidates all previous ContainerChild references. Using a stale reference throws a script error.*
