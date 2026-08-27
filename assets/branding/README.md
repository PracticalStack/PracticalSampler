# Practical Sampler mark

`practical-sampler-mark.svg` is the vector source for the Practical Sampler PS mark.

The mark uses the same core palette as PracticalWeb:

- Ink: `#181d21`
- Paper: `#f4f0e8`
- Teal: `#1c6c58`
- Orange: `#b56015`

Generated application assets:

- `practical-sampler-icon-32.png` — small JUCE/Windows icon source
- `practical-sampler-icon-256.png` — large JUCE/Windows icon source
- `practical-sampler.ico` — multi-resolution Windows icon (16, 24, 32, 48, 64, 128, 256)

PracticalWeb carries the same SVG at `public/assets/practical-sampler-mark.svg`, a 180 px touch icon,
and the multi-resolution ICO at `public/favicon.ico`.

Keep the glyph geometry and outer padding unchanged when regenerating small icons; they are tuned to remain legible at favicon and taskbar sizes.
