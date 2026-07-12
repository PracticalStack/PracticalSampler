## random

**Examples:**

```javascript:bipolar-random-offset
// Title: Bipolar random offset clamped to a normalised range
// Context: Randomizing a parameter value by a controlled amount
// requires converting the unipolar [0, 1) output to a bipolar
// [-1, 1) range, scaling by the randomization amount, then
// clamping back to valid bounds.

inline function randomizeValue(currentValue, amount)
{
    // Scale Math.random() from [0, 1) to [-1, 1) for bipolar offset
    local offset = amount * (2.0 * Math.random() - 1.0);
    return Math.range(currentValue + offset, 0.0, 1.0);
}

var currentX = 0.5;
var currentY = 0.5;

// Randomize XY pad positions
var newX = randomizeValue(currentX, 0.3);
var newY = randomizeValue(currentY, 0.3);
```
```json:testMetadata:bipolar-random-offset
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "newX >= 0.0 && newX <= 1.0", "value": true},
    {"type": "REPL", "expression": "newY >= 0.0 && newY <= 1.0", "value": true}
  ]
}
```
