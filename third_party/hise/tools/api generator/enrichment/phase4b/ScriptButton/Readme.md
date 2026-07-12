ScriptButton (object)
Obtain via: Content.addButton(name, x, y)

Toggle or momentary button component with filmstrip image support, radio group
mutual exclusion, and FloatingTile popup attachment. Binary on/off value (0 or 1)
with fixed range -- min/max properties are deactivated.

Common mistakes:
  - Setting numStrips to a value other than 2 or 6 -- silently falls back to
    the default skin with no error.
  - Passing an incomplete position array to setPopupData (e.g. [10, 20]) --
    must be [x, y, w, h] or a script error is thrown.
  - Expecting MIDI learn to work when saveInPreset is false -- saveInPreset
    being false disables MIDI learn regardless of enableMidiLearn setting.

Example:
  const var btn = Content.addButton("MyButton", 10, 10);
  btn.set("text", "Enable");

Methods (35):
  addToMacroControl               changed
  fadeComponent                   get
  getAllProperties                 getChildComponents
  getGlobalPositionX              getGlobalPositionY
  getHeight                       getId
  getLocalBounds                  getValue
  getValueNormalized              getWidth
  grabFocus                       loseFocus
  sendRepaintMessage              set
  setConsumedKeyPresses           setControlCallback
  setKeyPressCallback             setLocalLookAndFeel
  setPopupData                    setPosition
  setPropertiesFromJSON           setStyleSheetClass
  setStyleSheetProperty           setStyleSheetPseudoState
  setTooltip                      setValue
  setValueNormalized              setValueWithUndo
  setZLevel                       showControl
  updateValueFromProcessorConnection
