## attachToComponentMouseEvents

**Examples:**

```javascript:drag-bypass-pattern
// Title: Drag-bypass pattern - suspending parameter sync during user interaction
// Context: When a broadcaster monitors module parameters to update UI knobs,
// the user dragging those same knobs causes a feedback loop (the parameter
// change from user input triggers the broadcaster, which sets the knob value,
// which fights with the drag). This paired-broadcaster pattern solves it.

// --- setup ---
const var Attack = Content.addKnob("Attack", 0, 0);
Attack.set("saveInPreset", false);
const var Decay = Content.addKnob("Decay", 150, 0);
Decay.set("saveInPreset", false);
const var Sustain = Content.addKnob("Sustain", 300, 0);
Sustain.set("saveInPreset", false);
const var Release = Content.addKnob("Release", 450, 0);
Release.set("saveInPreset", false);
// --- end setup ---

const var paramSync = Engine.createBroadcaster({
    "id": "EnvelopeSync",
    "args": ["processorId", "parameterId", "value"]
});

const var dragGuard = Engine.createBroadcaster({
    "id": "DragGuard",
    "args": ["component", "event"]
});

const var envKnobs = [Attack, Decay, Sustain, Release];

// The drag guard watches mouse events on the knobs
dragGuard.attachToComponentMouseEvents(envKnobs, "Clicks Only", "knobInteraction");

// On mouse click, bypass the parameter sync broadcaster
dragGuard.addListener(paramSync, "bypass", function(component, event)
{
    if (isDefined(event.clicked))
        this.setBypassed(true, false, SyncNotification);
});

// 100ms after mouse release, re-enable parameter sync
dragGuard.addDelayedListener(100, paramSync, "reactivate", function(component, event)
{
    if (isDefined(event.mouseUp))
        this.setBypassed(false, false, SyncNotification);
});
```
```json:testMetadata:drag-bypass-pattern
{
  "testable": false,
  "skipReason": "Mouse events require physical user interaction that cannot be triggered programmatically from script"
}
```

```javascript:lazy-loading-on-hover
// Title: Lazy loading - trigger resource loading on first user interaction
// Context: Defer heavy initialization until the plugin window is actually
// opened and the user hovers over the interface.

// --- setup ---
const var BackgroundPanel = Content.addPanel("BackgroundPanel", 0, 0);
BackgroundPanel.set("width", 200);
BackgroundPanel.set("height", 200);
BackgroundPanel.set("saveInPreset", false);
const var HeaderPanel = Content.addPanel("HeaderPanel", 0, 200);
HeaderPanel.set("width", 200);
HeaderPanel.set("height", 50);
HeaderPanel.set("saveInPreset", false);
// --- end setup ---

const var lazyLoader = Engine.createBroadcaster({
    "id": "LazyLoader",
    "args": ["component", "event"],
    "tags": ["preloading"]
});

lazyLoader.attachToComponentMouseEvents(
    [BackgroundPanel, HeaderPanel],
    "Clicks & Hover",
    "rootHover"
);

lazyLoader.addListener("", "loadAndDisable", function(component, event)
{
    // Permanently disable after first trigger
    lazyLoader.setBypassed(true, false, false);

    // Trigger deferred sample loading here
    Console.print("Loading resources on first interaction...");
});
```
```json:testMetadata:lazy-loading-on-hover
{
  "testable": false,
  "skipReason": "Mouse events require physical user interaction (hover) that cannot be triggered programmatically from script"
}
```
