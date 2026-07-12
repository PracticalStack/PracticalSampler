Begins a new layer that composites using the specified blend mode. All subsequent draw calls render to an offscreen image until `endLayer()` is called, at which point the offscreen image is composited onto the parent canvas using the specified blend mode and alpha. Post-processing effects can also be applied to blend layers before ending them.

The blend mode must be one of 25 supported string values (case-sensitive):

| Blend Mode | Description |
|------------|-------------|
| `"Normal"` | Standard alpha compositing |
| `"Multiply"` | Multiplies pixel values, darkening the result |
| `"Screen"` | Inverse of Multiply, lightening the result |
| `"Overlay"` | Combines Multiply and Screen based on base brightness |
| `"SoftLight"` | Subtle version of Overlay with gentler transitions |
| `"HardLight"` | Combines Multiply and Screen based on top layer brightness |
| `"ColorDodge"` | Brightens the base colour to reflect the blend layer |
| `"ColorBurn"` | Darkens the base colour to reflect the blend layer |
| `"Lighten"` | Keeps the lighter pixel of the two layers |
| `"Darken"` | Keeps the darker pixel of the two layers |
| `"Add"` | Adds pixel values, brightening the result |
| `"Subtract"` | Subtracts the top layer from the bottom |
| `"Difference"` | Absolute difference between the two layers |
| `"Exclusion"` | Similar to Difference but with lower contrast |
| `"Negation"` | Inverted difference, producing a softer contrast effect |
| `"Average"` | Averages the pixel values of both layers |
| `"LinearDodge"` | Same as Add |
| `"LinearBurn"` | Adds pixel values then subtracts white |
| `"LinearLight"` | Combines LinearDodge and LinearBurn based on blend brightness |
| `"VividLight"` | Combines ColorDodge and ColorBurn based on blend brightness |
| `"PinLight"` | Replaces pixels based on brightness comparison |
| `"HardMix"` | Reduces colours to pure black or white |
| `"Reflect"` | Produces a bright reflective effect |
| `"Glow"` | Inverse of Reflect, applied to the base |
| `"Phoenix"` | Adds the minimum and subtracts the maximum of each channel |

> [!Warning:Invalid blend mode fails silently] An invalid blend mode string causes the method to silently fail - the layer is not created and no error is reported.
