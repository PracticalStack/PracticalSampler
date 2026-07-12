Content (namespace)

Top-level UI factory and namespace for creating components, layout objects, and
managing the script interface. Root factory for the entire HiseScript UI system:
component creation (buttons, knobs, panels, etc.) during onInit, utility object
factories (Path, Shader, SVG, MarkdownRenderer, LookAndFeel), and interface-level
configuration (size, tooltips, key press callbacks, value popups).

Complexity tiers:
  1. Basic interface: makeFrontInterface, addKnob/addButton/addPanel, getComponent.
     Every plugin needs these. Cache all component refs as const var at init.
  2. Styled interface: + createLocalLookAndFeel, createPath, setValuePopupData.
     Most commercial plugins reach this tier for visual polish.
  3. Interactive interface: + showModalTextInput, setKeyPressCallback,
     createMarkdownRenderer, getAllComponents. Preset naming, keyboard shortcuts,
     batch component operations.
  4. Advanced rendering: + createShader, createSVG, createScreenshot,
     setUseHighResolutionForPanels. GPU shaders, SVG icons, screenshot automation.

Practical defaults:
  - Always call Content.makeFrontInterface(width, height) as the very first line
    of onInit. Everything else depends on it.
  - Use Content.setUseHighResolutionForPanels(true) immediately after
    makeFrontInterface if any ScriptPanel uses custom paint routines. Without this,
    panels look blurry on Retina/HiDPI displays.
  - Cache every component reference as const var at init time. Never call
    Content.getComponent() repeatedly inside callbacks -- it performs a linear
    search each time.
  - Use Content.getAllComponents("Pattern.*") to batch-retrieve components sharing
    a naming convention, then iterate to assign a shared LAF or callback.
  - Store Content.createPath() results in const var at namespace scope. Paths are
    immutable once loaded -- create once, reuse across paint calls.

Common mistakes:
  - Calling Content.addKnob/addButton/etc. outside of onInit -- throws a script
    error. Component creation is only allowed during onInit.
  - Calling Content.getComponent() inside timer callbacks or paint routines --
    linear search on every call. Cache the reference at init time instead.
  - Creating paths inside paint routines via Content.createPath() -- allocates a
    new object on every repaint. Create once at init scope.
  - Calling Content.setToolbarProperties() -- deprecated since 2017, always throws
    a script error.

Example:
  // Content is a built-in namespace, no variable creation needed
  Content.makeFrontInterface(900, 600);
  const var knob1 = Content.addKnob("Volume", 10, 10);
  const var btn1 = Content.addButton("Bypass", 150, 10);

Methods (48):
  addAudioWaveform             addButton
  addComboBox                  addDynamicContainer
  addFloatingTile              addImage
  addKnob                      addLabel
  addMultipageDialog           addPanel
  addSliderPack                addTable
  addViewport                  addVisualGuide
  addWebView                   callAfterDelay
  componentExists              createLocalLookAndFeel
  createMarkdownRenderer       createPath
  createScreenshot             createShader
  createSVG                    getAllComponents
  getComponent                 getComponentUnderDrag
  getComponentUnderMouse       getCurrentTooltip
  getInterfaceSize             getScreenBounds
  isCtrlDown                   isMouseDown
  makeFrontInterface           makeFullScreenInterface
  refreshDragImage             restoreAllControlsFromPreset
  setContentTooltip            setHeight
  setKeyPressCallback          setName
  setPropertiesFromJSON        setSuspendTimerCallback
  setUpdateExistingPosition    setUseHighResolutionForPanels
  setValuePopupData            setWidth
  showModalTextInput           storeAllControlsAsPreset
