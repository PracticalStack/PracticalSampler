ScriptPanel (object)
Obtain via: Content.addPanel(name, x, y)

Scriptable panel with custom paint routines, mouse/timer callbacks, drag-and-drop,
popups, child panel hierarchies, image loading, and Lottie animation. The most
versatile UI component in HISE, providing a blank canvas for custom drawing.

Constants:
  data = {}    Persistent DynamicObject for storing arbitrary per-panel state

Complexity tiers:
  1. Static container: set, showControl, get. Page navigation via visibility toggling.
  2. Background image: + loadImage, setImage. Static image display without paint routine.
  3. Simple painted panel: + setPaintRoutine, repaint. Basic shape/text drawing triggered externally.
  4. Timer-driven visualizer: + setTimerCallback, startTimer. Polled values stored in data, repainted periodically.
  5. Interactive control: + setMouseCallback with allowCallbacks. Mouse updates data, triggers repaint.
  6. Full-featured panel: + setFileDropCallback, startInternalDrag, setLoadingCallback, setPanelValueWithUndo.

Practical defaults:
  - Use "Clicks Only" for simple click handlers. Escalate to "Clicks & Hover" for
    hover feedback, "Clicks, Hover & Dragging" for drag controls.
  - Set "opaque" to true on panels that fill their area with solid background.
  - Timer interval of 30ms for real-time meters, 50ms for less critical polling.
    Never below 15ms.
  - Store all per-panel runtime state in the data object. Access via this.data
    inside callbacks.
  - Cache Content.getComponent() references in const var at init time. Never call
    inside paint routines or timer callbacks.
  - Only call repaint() when the displayed value actually changed, not every timer tick.

Common mistakes:
  - Setting setMouseCallback without setting allowCallbacks first -- defaults to
    "No Callbacks", callback silently never fires.
  - Calling startTimer without setTimerCallback -- timer runs but nothing happens.
  - Calling Content.getComponent() inside paint/timer callbacks -- performs a lookup
    every call, thousands of unnecessary lookups per minute at 30ms intervals.
  - Using external var for panel state shared between callbacks -- use this.data
    instead, which is per-panel and accessible in all callbacks.
  - Setting allowCallbacks to "All Callbacks" for every panel -- fires on every
    mouse move, triggering unnecessary repaints.
  - Calling repaint() unconditionally in every timer tick -- compare new value to
    stored value first to avoid redundant paint cycles.

Example:
  const var pnl = Content.addPanel("Panel1", 0, 0);
  pnl.set("width", 200);
  pnl.set("height", 100);
  pnl.set("allowCallbacks", "Clicks & Hover");

  pnl.setPaintRoutine(function(g)
  {
      g.fillAll(0xFF222222);
      g.setColour(0xFFFFFFFF);
      g.drawRect([0, 0, this.getWidth(), this.getHeight()], 1.0);
  });

  pnl.setMouseCallback(function(event)
  {
      if (event.clicked)
          this.repaint();
  });

Methods (57):
  addChildPanel              addToMacroControl
  closeAsPopup               fadeComponent
  get                        getAllProperties
  getAnimationData           getChildComponents
  getChildPanelList          getGlobalPositionX
  getGlobalPositionY         getHeight
  getId                      getLocalBounds
  getParentPanel             getValue
  getWidth                   grabFocus
  isImageLoaded              isVisibleAsPopup
  loadImage                  loseFocus
  removeFromParent           repaint
  repaintImmediately         set
  setAnimation               setAnimationFrame
  setConsumedKeyPresses      setControlCallback
  setDraggingBounds          setFileDropCallback
  setImage                   setIsModalPopup
  setKeyPressCallback        setLoadingCallback
  setLocalLookAndFeel        setMouseCallback
  setMouseCursor             setPaintRoutine
  setPanelValueWithUndo      setPopupData
  setPosition                setStyleSheetClass
  setStyleSheetProperty      setStyleSheetPseudoState
  setTimerCallback           setTooltip
  setValue                   setValueWithUndo
  setZLevel                  showAsPopup
  showControl                startExternalFileDrag
  startInternalDrag          unloadAllImages
  updateValueFromProcessorConnection
