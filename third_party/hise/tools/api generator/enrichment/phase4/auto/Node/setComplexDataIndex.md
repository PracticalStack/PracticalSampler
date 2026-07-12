Changes which external data slot a node references for a specific data type. The node must be a data-consuming node (one that uses tables, slider packs, or audio files). Returns true on success, false if the data type is not recognised or the slot is out of range.

Valid data type strings:

| dataType | Description |
|----------|-------------|
| `"Table"` | Lookup table curve data |
| `"SliderPack"` | Resizable float array |
| `"AudioFile"` | Multichannel audio file reference |
| `"FilterCoefficients"` | Filter coefficient display data |
| `"DisplayBuffer"` | Visualisation buffer for oscilloscope or spectrum display |

```javascript
// Point a convolution node to the second AudioFile slot
const var conv = nw.create("filters.convolution", "myConv");
conv.setComplexDataIndex("AudioFile", 0, 1);
```
