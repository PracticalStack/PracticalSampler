# ScriptImage -- Project Context

## Project Context

### Real-World Use Cases
- **Background and layer images**: The most common use of ScriptImage is displaying full-size background images, overlays, reflections, and decorative layers that form the visual foundation of a plugin's UI. Multiple ScriptImage components are stacked with z-ordering and parenting to build layered visual compositions -- a background plate, a gradient overlay, a scanline texture, and a reflection panel might all be separate ScriptImage components.
- **Logo and branding display**: ScriptImage is used for static logo placement and branding elements that don't require interaction or animation.
- **Filmstrip indicators**: ScriptImage's `offset` property enables filmstrip-style display where a tall stacked image contains multiple frames. This is used for BPM digit displays, step indicators, and LED-style status lights where each frame represents a different state. The component's value or a script sets the offset to select which frame to show.
- **Expansion-aware themed backgrounds**: Plugins with expansion packs use ScriptImage to swap background artwork dynamically when the active expansion changes. The expansion provides its own image list, and the ScriptImage's `fileName` is updated to show expansion-specific branding.

### Complexity Tiers
1. **Static display** (most common): Set `fileName` in the Interface Designer, optionally adjust `alpha`. No scripting required -- purely declarative.
2. **Dynamic image swapping**: Use `set("fileName", path)` or `setImageFile()` to change the displayed image at runtime based on expansion selection, preset category, or UI state changes.
3. **Filmstrip indicator**: Use the `offset` property with a vertically-stacked image to display different frames. Combine with a Broadcaster or control callback to update the visible frame based on state.

### Practical Defaults
- `saveInPreset` defaults to `false` for ScriptImage, which is correct for most uses -- background images and decorative elements should not be saved in user presets.
- Use `set("fileName", path)` rather than `setImageFile()` for consistency with the standard property API. Both achieve the same result.
- Prefer setting images through the Interface Designer when the image is static. Only use scripting when the image needs to change at runtime.

### Integration Patterns
- `ExpansionHandler.setExpansionCallback()` -> `ScriptImage.set("fileName", expansion.getImageList()[0])` -- swap background artwork when the active expansion changes.
- `Broadcaster.addComponentPropertyListener()` -> `ScriptImage` `fileName` property -- reactively update artwork based on external state (e.g., preset directory changes driving artwork updates).

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating a ScriptPanel just to display a static image via `loadImage()` + `setPaintRoutine()` | Using ScriptImage with `fileName` set in the Interface Designer | ScriptImage is purpose-built for static image display. ScriptPanel adds unnecessary overhead (paint callback, mouse handling infrastructure) when no custom drawing or interaction is needed. |
