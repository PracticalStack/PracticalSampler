# ScriptImage -- Class Analysis

## Brief
Static image display component with filmstrip offset, alpha, blend modes, and popup menus.

## Purpose
ScriptImage displays a single image from the project's image pool on the interface. It supports vertical filmstrip-style offset for showing different frames of a stacked image, adjustable opacity via alpha, and 24 Photoshop-style blend modes powered by the gin library. It can optionally act as a clickable element with popup context menus, where the selected menu item index becomes the component's value.

## obtainedVia
`Content.addImage(name, x, y)` or via the Interface Designer.

## minimalObjectToken
img

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `img.setImageFile("myImage.png", true)` expecting disk reload | `img.setImageFile("{PROJECT_FOLDER}myImage.png", false)` | The `forceUseRealFile` parameter is currently ignored in the implementation; images are always loaded through the pool/expansion handler. Use the standard pool reference path format. |

## codeExample
```javascript
const var img = Content.addImage("MyImage", 0, 0);
img.set("fileName", "{PROJECT_FOLDER}myImage.png");
img.set("alpha", 0.8);
```

## Alternatives
Use ScriptPanel when you need custom drawing, mouse interaction, or animation instead of simple image display.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: ScriptImage is a simple display component with no timeline dependencies, no silent-failure preconditions beyond valid image paths (which are already handled by the pool system returning empty images), and no complex state interactions.
