Returns a direct buffer reference to the currently bound slider-pack data.

Use it for fast indexed reads or script-side processing of the full lane state.

> [!Warning:Direct writes bypass change callbacks] Writing to the returned buffer directly bypasses the normal `setAllValues` and `setSliderAtIndex` callback flow.
