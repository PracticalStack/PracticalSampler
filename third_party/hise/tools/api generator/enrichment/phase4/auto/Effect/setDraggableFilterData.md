Configures the interactive filter visualisation for a DraggableFilterPanel floating tile. Since HISE 5.0, this works with several effect types beyond the original Parametric EQ:

- Parametric EQ
- Filter (Polyphonic Filter)
- Hardcoded Master FX
- HardcodedPolyphonicFX
- Script FX
- Polyphonic Script FX

For the Parametric EQ, the panel can deduce parameter indices automatically. For all other types, you must provide a JSON configuration object that tells the panel how to map filter band parameters. The panel calculates the parameter index for each band using the formula:

```
P = O + I * N + B
```

where `O` is `FirstBandOffset`, `I` is the band index, `N` is the length of `ParameterOrder`, and `B` is the index of the target parameter within `ParameterOrder`.

The configuration object accepts these properties:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `NumFilterBands` | int | 1 | Fixed number of filter bands. Only the Parametric EQ supports a dynamic band count. |
| `FilterDataSlot` | int | 0 | Filter coefficient slot index for the visualisation display. |
| `FirstBandOffset` | int | 0 | Parameter offset before the first band's parameters begin (`O` in the formula). |
| `TypeList` | Array | see below | Filter type names shown in the right-click context menu on drag handles. |
| `ParameterOrder` | Array | see below | Parameter names per band in attribute order. The length defines `N` in the formula. |
| `FFTDisplayBufferIndex` | int | -1 | Display buffer index for FFT spectrum overlay. Set to -1 to disable; otherwise must point to a buffer connected to an `analyse.fft` node. |
| `DragActions` | Object | see below | Maps mouse interactions to parameter names. |

The `DragActions` object controls which parameter each mouse gesture adjusts:

| Property | Default | Description |
|----------|---------|-------------|
| `DragX` | `"Freq"` | Horizontal drag |
| `DragY` | `"Gain"` | Vertical drag |
| `ShiftDrag` | `"Q"` | Shift+drag |
| `DoubleClick` | `"Enabled"` | Double-click toggle |
| `RightClick` | `""` | Right-click (empty string for none) |

> [!Warning:Settings lost on recompile] Filter data properties are not persistently stored in the module tree. You must call this method in `onInit` for each effect that needs a custom configuration - the settings are lost on recompile.
