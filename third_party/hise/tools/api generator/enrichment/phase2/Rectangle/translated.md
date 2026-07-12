## translated

**Examples:**

```javascript:local-to-parent-coords
// Title: Converting component-local bounds to parent-relative coordinates
// Context: When rendering overlay graphics on a parent panel, child component
// bounds need to be translated by the child's position to get parent-relative
// coordinates.

inline function getParentRelativeBounds(component)
{
    return Rectangle(component.getLocalBounds(0))
        .translated(component.get("x"), component.get("y"));
}

// Use in a parent panel that draws outlines around its child components
const var parentPanel = Content.addPanel("ParentPanel", 0, 0);
parentPanel.set("width", 500);
parentPanel.set("height", 400);

const var childGraph = Content.addFloatingTile("ChildGraph", 100, 50);
childGraph.set("parentComponent", "ParentPanel");
childGraph.set("width", 200);
childGraph.set("height", 100);

// Store screen-relative bounds for drawing
parentPanel.data.childBounds = getParentRelativeBounds(childGraph);

parentPanel.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);

    // Draw a decorative border around the child component
    g.setColour(0xFF555555);
    g.drawRoundedRectangle(this.data.childBounds.reduced(-4), 6.0, 1);
});
```
```json:testMetadata:local-to-parent-coords
{
  "testable": false,
  "skipReason": "Paint routine requires panel rendering, cannot be tested standalone"
}
```

```javascript:method-chain-remove-translate
// Title: Method chaining with removeFromBottom and translated
// Context: Positioning a label area below a slider's drag region, centered
// and offset slightly downward. Demonstrates fluent chaining of
// removeFromBottom -> withSizeKeepingCentre -> translated.

var sliderBounds = Rectangle(0, 0, 80, 100);

// Slice the bottom 20px, center it to 100px wide, nudge 5px down
var labelArea = sliderBounds.removeFromBottom(20)
    .withSizeKeepingCentre(100, 20)
    .translated(0, 5);
```
```json:testMetadata:method-chain-remove-translate
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "labelArea.width", "value": 100},
    {"type": "REPL", "expression": "labelArea.height", "value": 20},
    {"type": "REPL", "expression": "labelArea.y", "value": 85},
    {"type": "REPL", "expression": "sliderBounds.height", "value": 80}
  ]
}
```
