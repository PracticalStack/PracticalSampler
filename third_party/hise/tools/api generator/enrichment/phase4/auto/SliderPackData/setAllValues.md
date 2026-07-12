Sets slider values from a single number, an Array, or a Buffer. A number sets all sliders to that value. An Array or Buffer sets the first N sliders from its elements, where N is the source length.

> [!Warning:Short arrays leave remaining values unchanged] When passing an Array or Buffer shorter than the slider count, only the first N sliders are updated. The remaining sliders keep their previous values. Ensure the source length matches `getNumSliders()` for a complete replacement.
