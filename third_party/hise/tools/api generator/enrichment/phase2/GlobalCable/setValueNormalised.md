## setValueNormalised

**Examples:**

```javascript:forwarding-slider-values-to
// Title: Forwarding slider values to DSP network via cable
// Set normalized cable value and verify
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("TestCable");

cable.setRange(0.0, 100.0);
cable.setValueNormalised(0.5);

Console.print("Normalised value: " + cable.getValueNormalised());
Console.print("Actual value: " + cable.getValue());
```
```json:testMetadata:forwarding-slider-values-to
{
  "testable": true,
  "verifyScript": {
    "type": "log-output",
    "values": [
      "Normalised value: 0.5",
      "Actual value: 50.0"
    ]
  }
}
```

