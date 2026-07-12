ScriptImage (object)
Obtain via: Content.addImage(name, x, y)

Static image display component with filmstrip offset, alpha blending, blend modes,
and popup context menus. Displays a single image from the project's image pool.
Supports vertical filmstrip-style offset for multi-frame stacked images, adjustable
opacity, and 24 Photoshop-style blend modes (gin library).

Complexity tiers:
  1. Static display: set fileName in Interface Designer, optionally adjust alpha.
     No scripting required -- purely declarative.
  2. Dynamic image swapping: set, setImageFile. Change displayed image at runtime
     based on expansion selection, preset category, or UI state.
  3. Filmstrip indicator: + offset property with vertically-stacked image. Combine
     with Broadcaster or control callback to update visible frame based on state.

Practical defaults:
  - saveInPreset defaults to false for ScriptImage -- correct for most uses
    (background images and decorative elements should not be saved in presets).
  - Use set("fileName", path) rather than setImageFile() for consistency with the
    standard property API. Both achieve the same result.
  - Prefer setting images through the Interface Designer when the image is static.
    Only use scripting when the image needs to change at runtime.

Common mistakes:
  - Using ScriptPanel with loadImage() + setPaintRoutine() just to display a static
    image -- ScriptImage is purpose-built for this and avoids paint callback overhead.
  - Passing forceUseRealFile=true to setImageFile() expecting disk reload -- the
    parameter is ignored; images always load through the pool/expansion handler.

Example:
  const var img = Content.addImage("MyImage", 0, 0);
  img.set("fileName", "{PROJECT_FOLDER}myImage.png");
  img.set("alpha", 0.8);

Methods (32):
  changed                          fadeComponent
  get                              getAllProperties
  getChildComponents               getGlobalPositionX
  getGlobalPositionY               getHeight
  getId                            getLocalBounds
  getValue                         getWidth
  grabFocus                        loseFocus
  sendRepaintMessage               set
  setAlpha                         setConsumedKeyPresses
  setControlCallback               setImageFile
  setKeyPressCallback              setLocalLookAndFeel
  setPosition                      setStyleSheetClass
  setStyleSheetProperty            setStyleSheetPseudoState
  setTooltip                       setValue
  setValueWithUndo                 setZLevel
  showControl                      updateValueFromProcessorConnection
