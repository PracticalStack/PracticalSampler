Maps a 0..1 input to the slider's actual range using current midpoint/skew settings, then writes that value. Midpoint skew is only applied when `middlePosition` resolves to a numeric value inside the active range. This is the preferred method when custom gesture surfaces or modulators work in normalised space.

For vertically inverted gesture surfaces, invert the input (`1.0 - n`) before calling this method so upward motion still feels like increasing value.

> [!Warning:Invalid range prevents value update] If range settings are invalid, the value is not updated.

> [!Warning:Use disabled string for no-skew] Do not rely on legacy `-1` midpoint values to disable skew in all ranges. Use `setMidPoint("disabled")` for explicit no-skew behaviour.
