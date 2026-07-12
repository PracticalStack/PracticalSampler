ScriptTable (object)
Obtain via: Content.addTable(componentId, x, y)

Table curve editor component with complex-data binding for ExternalData::Table.
Use it to edit transfer curves in the UI, then read the same curve in runtime logic.
Supports internal ownership, processor-slot binding, and explicit shared-table binding.

Complexity tiers:
  1. Single-curve mapper: getTableValue, setTablePoint. One UI curve used as a transfer function.
  2. Shared data workflow: + registerAtParent, referToData. Share one table source across UI and processing paths.
  3. Full editor system: + setLocalLookAndFeel, setTablePopupFunction, setMouseHandlingProperties. Build dense multi-table modulation editors.

Practical defaults:
  - Normalize MIDI-domain inputs before lookup (eg. velocity / 127.0) before calling getTableValue.
  - Call registerAtParent once in init and cache the returned handle for runtime use.
  - When styling tables, register drawTableBackground, drawTablePath, drawTablePoint, and drawTableRuler on one LAF object.
  - Use setTablePopupFunction(false) or an empty formatter when drag popups obscure dense editor layouts.

Interaction:
  - Hold Ctrl and scroll the mouse wheel over a line segment to adjust its curve between two points.
  - Dragging table points does NOT trigger the control callback. Call changed() explicitly to fire it from script.

Styling:
  - Set customColours to true to enable flat-style rendering with standard colour properties (bgColour, itemColour, etc.).

Complex data chain:

![Table Data Chain](topology_complex-table-data-chain.svg)

  - TableProcessor selects the module that owns one or more table slots.
  - Table is the complex-data handle for one slot within that module.
  - ScriptTable displays or edits one selected slot in the UI.

  Use the binding properties separately:
  - processorId selects the owning processor.
  - tableIndex selects which table slot inside that processor should be displayed.

  This is not the normal parameter binding path. parameterId targets processor
  parameters, while table-slot binding uses tableIndex instead.

Common mistakes:
  - Passing raw MIDI values to getTableValue -- lookup is normalized-domain, so unnormalized input collapses curve behavior.
  - Calling registerAtParent during note callbacks -- setup work belongs in init, not playback paths.
  - Passing non-array input to setSnapValues -- script error plus confusing state because wrapper update path still runs.
  - Calling referToData with non-table complex data -- type mismatch prevents binding.
  - Calling setKeyPressCallback before setConsumedKeyPresses -- runtime reports an error and callback is not armed.

Example:
  const var st = Content.addTable("EnvCurve", 20, 20);
  st.setTablePoint(1, 0.5, 0.8, 0.5);
  st.setMouseHandlingProperties({ "allowSwap": false, "numSteps": 8 });

Methods (41):
  addTablePoint                  changed
  fadeComponent                  get
  getAllProperties               getChildComponents
  getGlobalPositionX             getGlobalPositionY
  getHeight                      getId
  getLocalBounds                 getTableValue
  getValue                       getValueNormalized
  getWidth                       grabFocus
  loseFocus                      referToData
  registerAtParent               reset
  sendRepaintMessage             set
  setConsumedKeyPresses          setControlCallback
  setKeyPressCallback            setLocalLookAndFeel
  setMouseHandlingProperties     setPosition
  setSnapValues                  setStyleSheetClass
  setStyleSheetProperty          setStyleSheetPseudoState
  setTablePoint                  setTablePopupFunction
  setTooltip                     setValue
  setValueNormalized             setValueWithUndo
  setZLevel                      showControl
  updateValueFromProcessorConnection
