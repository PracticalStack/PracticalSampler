## constrainedWithin

**Examples:**

```javascript:tooltip-constrained-to-parent
// Title: Keeping a floating tooltip within parent bounds
// Context: Positioning a tooltip or popup element near a target control,
// then constraining it so it does not overflow the parent panel's bounds.

const var TOOLTIP_WIDTH = 110;
const var TOOLTIP_HEIGHT = 24;

inline function getTooltipBounds(targetBounds, parentBounds)
{
    // Position tooltip centered below the target, offset 5px down
    var tooltip = Rectangle(
        targetBounds.x + (targetBounds.width - TOOLTIP_WIDTH) / 2,
        targetBounds.y + targetBounds.height + 5,
        TOOLTIP_WIDTH,
        TOOLTIP_HEIGHT
    );

    // Clamp to parent bounds so it doesn't overflow the edges
    return tooltip.constrainedWithin(parentBounds);
}

var target = Rectangle(350, 10, 40, 40);
var parent = Rectangle(0, 0, 400, 300);
var tip = getTooltipBounds(target, parent);
// tooltip slides left to stay within the 400px-wide parent
```
```json:testMetadata:tooltip-constrained-to-parent
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "tip.width", "value": 110},
    {"type": "REPL", "expression": "tip.height", "value": 24},
    {"type": "REPL", "expression": "tip.x + tip.width <= 400", "value": true},
    {"type": "REPL", "expression": "tip.y", "value": 55}
  ]
}
```
