## setMouseCallback

**Examples:**

```javascript:threshold-drag-control
// Title: Interactive threshold control with drag, double-click reset, and hover feedback
// Context: A display panel where the user drags a vertical line to set a threshold.
// Demonstrates the full mouse event state machine: click sets initial value,
// drag updates continuously, double-click resets to default, hover changes cursor.

// --- setup ---
Content.addKnob("ThresholdKnob", 0, 0);
// --- end setup ---

const var display = Content.addPanel("ThresholdDisplay", 0, 0);
display.set("width", 300);
display.set("height", 100);
display.set("allowCallbacks", "All Callbacks");

display.data.threshold = 0.5;
display.data.hover = false;
display.data.dragging = false;
display.data.downValue = 0.0;

const var thresholdKnob = Content.getComponent("ThresholdKnob");

display.setMouseCallback(function(event)
{
    this.data.hover = event.hover;

    if (event.clicked)
    {
        // Store starting value for drag offset calculation
        this.data.downValue = event.x / this.getWidth();
        this.data.threshold = this.data.downValue;
        this.data.dragging = true;
    }
    else if (event.doubleClick)
    {
        // Reset to default on double-click
        this.data.threshold = 0.5;
        thresholdKnob.setValue(0.5);
        thresholdKnob.changed();
    }
    else if (event.drag)
    {
        // Continuous update during drag
        local delta = event.dragX / this.getWidth();
        local newValue = Math.range(this.data.downValue + delta, 0.0, 1.0);
        this.data.threshold = newValue;
    }
    else if (event.mouseUp)
    {
        this.data.dragging = false;

        // Commit final value to the connected knob
        thresholdKnob.setValueNormalized(this.data.threshold);
        thresholdKnob.changed();
    }

    this.repaint();
});

display.setPaintRoutine(function(g)
{
    g.fillAll(0xFF27282A);

    // Draw threshold line
    local x = this.data.threshold * this.getWidth();
    local alpha = this.data.hover ? 0.8 : 0.4;
    g.setColour(Colours.withAlpha(this.get("itemColour"), alpha));
    g.fillRect([x, 0, 2, this.getHeight()]);

    // Draw border
    g.setColour(0xFF27282A);
    g.drawRoundedRectangle(this.getLocalBounds(0), 3.0, 2);
});

// --- test-only ---
Console.testCallback(display, "setMouseCallback", {
    "clicked": true, "x": 225, "y": 50,
    "mouseDownX": 225, "mouseDownY": 50,
    "hover": false, "drag": false, "mouseUp": false,
    "doubleClick": false, "rightClick": false
});
// --- end test-only ---
```
```json:testMetadata:threshold-drag-control
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "display.data.threshold", "value": 0.75},
    {"type": "REPL", "expression": "display.data.dragging", "value": 1}
  ]
}
```

```javascript:modal-dismiss-outside-click
// Title: Modal dialog dismiss on outside click
// Context: A full-interface overlay panel that dismisses when the user
// clicks outside the dialog box area. Uses the data object to store
// the box bounds for hit testing.

const var modal = Content.addPanel("ModalOverlay", 0, 0);
modal.set("width", 800);
modal.set("height", 600);
modal.set("visible", false);
modal.set("allowCallbacks", "Clicks Only");

// Store the dialog box bounds for hit-testing
modal.data.boxBounds = [175, 193, 450, 214];

modal.setMouseCallback(function(event)
{
    if (event.clicked)
    {
        // Dismiss if click is outside the dialog box
        if (!Rect.contains(this.data.boxBounds, [event.x, event.y]))
            this.set("visible", false);
    }
});

modal.setPaintRoutine(function(g)
{
    // Semi-transparent background
    g.fillAll(0x66000000);

    // Draw dialog box
    g.setColour(0xFFDDDDDD);
    g.fillRoundedRectangle(this.data.boxBounds, 10);
});
```
```json:testMetadata:modal-dismiss-outside-click
{
  "testable": false,
  "skipReason": "Requires visual UI interaction to show modal popup before testing dismiss behavior"
}
```

**Pitfalls:**
- The `event.drag` flag only fires when the mouse moves with the button held. For a click-then-drag control, always store the initial click position (from `event.clicked`) in `this.data` and compute the delta in the `event.drag` handler using `event.dragX`/`event.dragY`.
- The `event.hover` flag requires `allowCallbacks` to be at least `"Clicks & Hover"`. Setting `"Clicks Only"` silently suppresses hover events without any error.
