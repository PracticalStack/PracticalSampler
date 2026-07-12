Attaches the broadcaster to samplemap events on one or more sampler modules. The `eventTypes` parameter accepts:

| Event Type | Data Argument |
|---|---|
| `"SampleMapChanged"` | The samplemap reference string |
| `"SamplesAddedOrRemoved"` | The integer sound count |
| Integer (sample property index) | JSON object with `sound`, `id`, and `value` properties |

For individual sample property changes, pass integer constants from the `Sampler` API as the event type instead of a string. In this mode the `data` argument is a JSON object with `sound` (a `Sample` reference), `id` (the property constant), and `value` (the new property value):

```js
const var bc = Engine.createBroadcaster({
    id: "keyRangeWatcher",
    args: ["eventType", "samplerId", "data"]
});

bc.attachToSampleMap("Sampler1", [Sampler.LoKey, Sampler.HiKey], "");

bc.addListener("", "logKeyChanges", function(eventType, samplerId, data)
{
    if (data.id == Sampler.LoKey)
        Console.print("Low key changed to " + data.value);
    if (data.id == Sampler.HiKey)
        Console.print("High key changed to " + data.value);
});
```

> [!Tip:Preferred over setLoadingCallback for sample maps] Using a broadcaster for sample map changes is the recommended approach going forward and replaces `ScriptPanel.setLoadingCallback()` for this task.

Queue mode is automatically enabled when monitoring multiple samplers or event types.
