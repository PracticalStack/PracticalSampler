Sets the midpoint source used for skewed normalised mapping. You can pass a numeric value (including numeric strings like `"1.5"`) or the explicit sentinel string `"disabled"` for no-skew behaviour.

```javascript
const var sl = Content.addKnob("Threshold", 0, 0);
sl.setRange(-48.0, 0.0, 0.1);
sl.setMidPoint(-12.0);      // Numeric midpoint route
sl.setMidPoint("disabled"); // Explicit no-skew route
```

> [!Warning:Legacy -1 now treated as numeric value] Legacy `-1` midpoint values are treated as numeric now. If your range contains `-1`, skew will be applied. Use `"disabled"` for an explicit no-skew state.
