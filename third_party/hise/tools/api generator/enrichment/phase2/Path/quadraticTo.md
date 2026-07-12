## quadraticTo

**Examples:**

```javascript:speech-bubble-path
// Title: Tooltip speech bubble with rounded corners
// Context: quadraticTo creates smooth rounded corners on custom
// shapes that addRoundedRectangle cannot express - like a speech
// bubble with an arrow pointer on one side.

const var SHADOW = 10;
const var CORNER = 3;

inline function createBubblePath(width, height)
{
    local p = Content.createPath();
    local r = width - SHADOW;

    // Top edge with rounded corners
    p.startNewSubPath(SHADOW + CORNER, SHADOW);
    p.lineTo(r - CORNER, SHADOW);
    p.quadraticTo(r, SHADOW, r, SHADOW + CORNER);

    // Right edge with arrow pointer
    p.lineTo(r, 20);
    p.lineTo(r + 10, 30);
    p.lineTo(r, 40);
    p.lineTo(r, height - SHADOW - CORNER);

    // Bottom-right corner
    p.quadraticTo(r, height - SHADOW,
                  r - CORNER, height - SHADOW);

    // Bottom edge
    p.lineTo(SHADOW + CORNER, height - SHADOW);

    // Bottom-left corner
    p.quadraticTo(SHADOW, height - SHADOW,
                  SHADOW, height - CORNER - SHADOW);

    // Left edge back to start
    p.lineTo(SHADOW, SHADOW + CORNER);
    p.quadraticTo(SHADOW, SHADOW,
                  SHADOW + CORNER, SHADOW);

    p.closeSubPath();
    return p;
}

const var bubble = createBubblePath(200, 100);
```
```json:testMetadata:speech-bubble-path
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "bubble.getLength() > 0", "value": true},
    {"type": "REPL", "expression": "bubble.contains([100, 50])", "value": true}
  ]
}
```

```javascript:envelope-curve-segment
// Title: Envelope curve segment with adjustable curvature
// Context: AHDSR envelope editors use quadraticTo to draw curved
// attack, decay, and release segments. The control point position
// is interpolated between start and end based on a curve parameter
// (0.0 = linear, approaching 1.0 = exponential).

const var p = Content.createPath();
var x = 0.0;
var y = 1.0;  // start at bottom
var cx = 0.0;
var cy = 1.0;

p.startNewSubPath(x, y);

// Attack segment with curve factor
var attackTime = 0.3;
var attackLevel = 0.0;  // 0.0 = top
var curveFactor = 0.5;  // 0 = linear, 1 = extreme curve

x = attackTime;
y = attackLevel;

// Interpolate control point between start and end
cy = curveFactor * cy + (1.0 - curveFactor) * y;
cx = (1.0 - curveFactor) * cx + curveFactor * x;

p.quadraticTo(cx, cy, x, y);
```
```json:testMetadata:envelope-curve-segment
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "p.getLength() > 0.3", "value": true},
    {"type": "REPL", "expression": "isDefined(p.getYAt(0.15))", "value": true}
  ]
}
```
