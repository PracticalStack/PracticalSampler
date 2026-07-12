Sets multiple node parameters from a JSON object. Each property key uses `nodeId.parameterId` format, and the value is a number. Matched parameters are updated with undo support.

```javascript
nw.setParameterDataFromJSON({
    "myGain.Gain": -6.0,
    "myOsc.Frequency": 440.0,
    "myFilter.Cutoff": 2000.0
});
```
