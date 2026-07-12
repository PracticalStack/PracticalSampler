## setTimerCallback

**Examples:**

```javascript:poll-audio-levels-meter
// Title: Polling audio levels for a peak meter
// Context: The most common Timer pattern -- poll processor levels and
// trigger panel repaints at a fixed interval for meter visualization.

const var meter = Content.getComponent("PeakMeterPanel");
const var synth = Synth.getChildSynth("Synth1");

const var DECAY = 0.77;
meter.data.level = 0.0;

meter.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);
    local h = this.getHeight() * this.data.level;
    g.setColour(0xFF00FF00);
    g.fillRect([0, this.getHeight() - h, this.getWidth(), h]);
});

const var tm = Engine.createTimerObject();

tm.setTimerCallback(function()
{
    local current = synth.getCurrentLevel(true);

    // Smoothed decay: fast attack, slow release
    if (current > meter.data.level)
        meter.data.level = current;
    else
        meter.data.level *= DECAY;

    meter.repaint();
});

tm.startTimer(50);
```
```json:testMetadata:poll-audio-levels-meter
{
  "testable": false,
  "skipReason": "Requires a child synth module for getCurrentLevel() and produces non-deterministic audio level output."
}
```

```javascript:deferred-sample-map-loading
// Title: Deferred sample map loading via polling
// Context: Control callbacks run on the audio thread where loadSampleMap()
// would cause hitches. The timer polls on the message thread instead.

const var sampler = Synth.getSampler("Sampler1");
const var selector = Content.getComponent("MapSelector");

reg lastIndex = 0;
reg currentIndex = 0;

const var loadTimer = Engine.createTimerObject();

loadTimer.setTimerCallback(function()
{
    if (lastIndex != currentIndex)
    {
        sampler.loadSampleMap(sampler.getSampleMapList()[currentIndex]);
        lastIndex = currentIndex;
    }
});

loadTimer.startTimer(50);

inline function onSelectorControl(component, value)
{
    // Just update the index -- the timer handles the actual loading
    currentIndex = parseInt(value - 1);
}

selector.setControlCallback(onSelectorControl);
```
```json:testMetadata:deferred-sample-map-loading
{
  "testable": false,
  "skipReason": "Requires a Sampler module with loaded sample maps."
}
```

```javascript:one-shot-delayed-action
// Title: One-shot self-stopping timer for delayed action
// Context: Use this.stopTimer() inside the callback to fire once after
// a delay, similar to setTimeout() in JavaScript.

// --- setup ---
Content.addLabel("StatusLabel", 0, 0);
// --- end setup ---

const var statusLabel = Content.getComponent("StatusLabel");

const var delayTimer = Engine.createTimerObject();

delayTimer.setTimerCallback(function()
{
    this.stopTimer();
    statusLabel.set("text", "");
});

// Call this from any event to show a temporary status message
inline function showStatus(text)
{
    statusLabel.set("text", text);
    delayTimer.startTimer(2000);
}

// --- test-only ---
showStatus("test");
delayTimer.startTimer(50);
// --- end test-only ---
```
```json:testMetadata:one-shot-delayed-action
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "delayTimer.isTimerRunning()", "value": false},
    {"type": "REPL", "expression": "statusLabel.get('text')", "value": ""}
  ]
}
```

**Pitfalls:**
- The callback receives zero arguments and `this` is set to the Timer instance itself. Use `this.stopTimer()` for self-stopping patterns rather than capturing the timer variable.
- Timer callbacks fire on the message thread, not the audio thread. They are safe for UI operations but not suitable for sample-accurate timing. For beat-synced callbacks, use `TransportHandler` instead.
