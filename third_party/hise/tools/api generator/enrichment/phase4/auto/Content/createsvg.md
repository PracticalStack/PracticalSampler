Creates an SVG object from base64-encoded, zstd-compressed SVG data (typically exported from the HISE SVG tools). Use the returned object with `Graphics.drawSVG()` in paint routines.

> [!Warning:SVG parsed asynchronously] The SVG is parsed asynchronously. The returned object may not be immediately valid for drawing - the internal drawable is set on the UI thread after XML parsing completes.