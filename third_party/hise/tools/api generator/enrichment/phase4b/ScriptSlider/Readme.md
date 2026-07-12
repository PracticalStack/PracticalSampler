ScriptSlider (object)
Obtain via: Content.addKnob("MySlider", x, y)

Single-value slider or knob component for editing numeric values in script UIs.
Supports mode-aware ranges and formatting, style changes, range helpers, modifier mappings, and matrix target integration.

Mode guide:
  setMode(...) affects both conversion and display formatting.
  - Frequency: frequency-oriented defaults for cutoff / oscillator tuning controls.
  - Decibel: dB-oriented defaults for gain, drive, threshold, and level controls.
  - Time: time-oriented defaults for envelope and delay timing parameters.
  - TempoSync: indexed tempo-division mode for synced rates and delay sync.
  - Linear: plain numeric mapping for generic controls.
  - Discrete: stepped integer-like mapping for selector-style controls.
  - Pan: centered bipolar mapping with left/right pan semantics.
  - NormalizedPercentage: 0..1 mapping displayed as percentages for mix / macro style controls.

  If old mode defaults are untouched, mode switches can migrate default range settings
  (min/max/step/suffix/midpoint) to the new mode profile.

Complexity tiers:
  1. Parameter-bound control: set, setRange, setMode, setControlCallback. Standard parameter editing.
  2. Interaction tuning: + createModifiers, setModifiers, setValuePopupFunction. Fine tune gestures and popup text.
  3. Deep integration: + setLocalLookAndFeel, connectToModulatedParameter, updateValueFromProcessorConnection. Matrix overlays and restore sync.

Practical defaults:
  - Reuse one modifiers object from createModifiers across slider collections for consistent interaction mapping.
  - Use one shared formatter with setValuePopupFunction for non-linear or semantic labels.
  - For custom gesture surfaces, use setValueNormalized then changed when callback-driven logic must run.
  - After processor state restore, call updateValueFromProcessorConnection on connected sliders.

Common mistakes:
  - Calling setMinValue/setMaxValue outside Range style -- range helper call is ignored and logs an error.
  - Using invalid mode strings (eg "Db") -- mode mapping falls back and behavior does not match expectation.
  - Using unknown modifier action names (eg "Reset") -- modifier mapping is not applied.
  - Using -1 as a universal midpoint-disable token -- use setMidPoint("disabled") for explicit no-skew behavior.
  - Driving setValueNormalized from custom UI and skipping changed -- callback and parameter workflows do not run.
  - Restoring module state without updateValueFromProcessorConnection -- slider UI becomes stale.

Example:
  const var sl = Content.addKnob("Drive", 20, 20);
  sl.setMode("Decibel");
  sl.setRange(-24.0, 12.0, 0.1);

Methods (47):
  addToMacroControl              changed                      connectToModulatedParameter
  contains                       createModifiers              fadeComponent
  get                            getAllProperties             getChildComponents
  getGlobalPositionX             getGlobalPositionY           getHeight
  getId                          getLocalBounds               getMaxValue
  getMinValue                    getValue                     getValueNormalized
  getWidth                       grabFocus                    loseFocus
  sendRepaintMessage             set                          setConsumedKeyPresses
  setControlCallback             setKeyPressCallback          setLocalLookAndFeel
  setMaxValue                    setMidPoint                  setMinValue
  setMode                        setModifiers                 setPosition
  setPropertiesFromJSON          setRange                     setStyle
  setStyleSheetClass             setStyleSheetProperty        setStyleSheetPseudoState
  setTooltip                     setValue                     setValueNormalized
  setValuePopupFunction          setValueWithUndo             setZLevel
  showControl                    updateValueFromProcessorConnection
