<!-- Diagram triage:
  - preset-load-sequence: RENDER (complex 10-step sequence with branching pre-callback paths)
  - automation-connection-types: RENDER (fan-in/fan-out topology with 3 connection types and 4 input sources)
  NOTE: SVG rendering not available in this session. Diagrams survive triage but are not rendered.
-->

# UserPresetHandler

UserPresetHandler customises the data handling for your project's user presets. It controls how preset data is saved and loaded, exposes named automation parameters to DAW hosts and MIDI controllers, and provides hooks into the preset load lifecycle. Create one with `Engine.createUserPresetHandler()`:

```js
const var uph = Engine.createUserPresetHandler();
```

The class supports two data models:

| Model | Activation | Save/Load |
|-------|-----------|-----------|
| Default | Automatic (no setup) | Serialises all `saveInPreset` component values |
| Custom | `setUseCustomUserPresetModel(load, save, persist)` | Script callbacks receive and return JSON |

For most projects, the default model handles preset serialisation automatically. Add lifecycle hooks with `setPreCallback` and `setPostCallback` to run logic before or after a preset loads - for example, updating a display or migrating older presets. When you need full control over the preset format (structured data, per-channel settings, or non-component state), switch to the custom data model.

The custom automation system lets you define named parameter slots that the DAW and MIDI controllers can target. Each slot specifies its range, display format, and connection targets - which can be module parameters, other automation slots (meta-parameters), or global routing cables. Use the `automationID` property on a UI component to dynamically bind it to a slot.

You can also pass a Broadcaster directly to `setPreCallback` or `setPostCallback` instead of a plain function. This turns the preset lifecycle into an event bus that multiple listeners can subscribe to independently.

> [!Tip:One instance, set custom model first] Best practice is to have only one UserPresetHandler instance in your project. Call `setUseCustomUserPresetModel` before `setCustomAutomation` - the custom data model is a prerequisite for custom automation.

## Common Mistakes

- **Enable custom model before setCustomAutomation**
  **Wrong:** `uph.setCustomAutomation(data)` without enabling the custom model first.
  **Right:** `uph.setUseCustomUserPresetModel(load, save, false); uph.setCustomAutomation(data);`
  *Custom automation requires the custom data model. Calling `setCustomAutomation` without it throws a script error.*

- **Check isInternalPresetLoad inside callbacks only**
  **Wrong:** Calling `uph.isInternalPresetLoad()` outside a pre/post callback.
  **Right:** Only call `isInternalPresetLoad()` inside `setPreCallback` or `setPostCallback`.
  *Outside those callbacks, the flag retains its value from the most recent load, which may be stale.*

- **Wrap single objects in array brackets**
  **Wrong:** Passing a single object to `updateAutomationValues`: `uph.updateAutomationValues({"id": "Vol", "value": 0.5}, ...)`.
  **Right:** Wrap in an array: `uph.updateAutomationValues([{"id": "Vol", "value": 0.5}], ...)`.
  *The method expects an Array. Passing an unwrapped object throws a script error.*

- **Limit host automation to essential parameters**
  **Wrong:** Setting `allowHostAutomation: true` on every automation slot in a large instrument.
  **Right:** Set `allowHostAutomation: false` on internal per-layer slots; only expose top-level parameters to the host.
  *Exposing hundreds of parameters to the DAW creates an unusable automation list for the end user.*
