Adds an arc (a section of an ellipse's outline) defined by a bounding rectangle and angular range. The arc is the most common Path primitive in practice - nearly every custom rotary knob uses `addArc` to draw background tracks and value indicators inside `drawRotarySlider` LAF callbacks.

Angles are measured in radians, clockwise from the 3 o'clock position. For a standard knob arc with a gap at the bottom, use a half-range of roughly 2.4 radians: `-ARC` to `+ARC` for the background track, and `-ARC` to `-ARC + 2.0 * ARC * valueNormalized` for the value arc.

> [!Warning:Anchor bounds before adding arc] When using `addArc` with normalised coordinates `[0, 0, 1, 1]` for later scaling via `drawPath`, the path's bounding box only covers the arc segment itself. Call `setBounds([0, 0, 1, 1])` before the arc to anchor the bounds to the full unit square, preventing skewed or misaligned rendering.
