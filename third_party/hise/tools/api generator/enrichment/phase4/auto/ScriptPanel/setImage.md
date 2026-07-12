Switches the panel to fixed image mode, bypassing the paint routine. Clips a region from a previously loaded image using the given offsets for filmstrip-style rendering. Either the x-offset or y-offset must be 0 - the non-zero offset selects the frame along the strip direction.

> [!Warning:Clears active paint routine] Calling `setImage()` clears any active paint routine. Call `setPaintRoutine()` again to return to custom drawing mode.
