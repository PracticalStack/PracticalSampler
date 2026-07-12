Content::createSVG(String base64Data) -> ScriptObject

Thread safety: UNSAFE -- heap-allocates an SVGObject, decompresses base64/zstd data, and parses XML asynchronously.
Creates an SVG object from base64-encoded, zstd-compressed SVG data. The SVG is parsed
asynchronously on the message thread. Use the returned object with Graphics.drawSVG()
in paint routines.

Required setup:
  const var svg = Content.createSVG(encodedSvgData);

Pair with:
  Graphics.drawSVG -- render the SVG in paint routines

Anti-patterns:
  - The SVG drawable is set asynchronously (SafeAsyncCall). The object may not be
    immediately valid after creation -- first paint call may render nothing if parsing
    has not completed yet.

Source:
  ScriptingApiContent.cpp  Content::createSVG()
    -> new SVGObject (heap allocation)
    -> base64 decode + zstd decompress
    -> SafeAsyncCall for XML parsing on message thread
