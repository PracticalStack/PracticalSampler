Same as `setAllValues()` but registers the operation with the undo manager, making the entire bulk update a single undoable action. Accepts a single number, Array, or Buffer. Use this for operations the user should be able to reverse - clearing, pasting, or randomising patterns.

> [!Warning:Short arrays leave remaining values unchanged] When passing an Array or Buffer shorter than the slider count, only the first N sliders are updated. Ensure the source length matches `getNumSliders()` for a complete replacement.
