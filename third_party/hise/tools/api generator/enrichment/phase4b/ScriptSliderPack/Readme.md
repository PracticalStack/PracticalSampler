ScriptSliderPack (object)
Obtain via: Content.addSliderPack(componentId, x, y)

Multi-slider array editor for SliderPackData values.
It can edit its internal data object, a processor external-data slot, or a referred ScriptSliderPackData / compatible complex-data source.
Use it for step-lane style editing, shared multi-view data editing, and bulk value operations.

Complexity tiers:
  1. Single-lane editor: set("sliderAmount", ...), setControlCallback, getSliderValueAt. Basic editable lane UI.
  2. Shared-data lane system: + referToData. Rebind one UI lane to different external datasets.
  3. Sequencer-scale lane management: + setAllValueChangeCausesCallback, setWidthArray, setLocalLookAndFeel. Dense lane editors with callback/load control and custom geometry.

Practical defaults:
  - Use set("mouseUpCallback", true) for step-lane editing so drag gestures commit once.
  - Disable bulk callbacks with setAllValueChangeCausesCallback(false) during pattern loads, then run one explicit downstream refresh.
  - Keep sliderAmount and setWidthArray updates in one code path so width-map size and lane count stay aligned.
  - Reuse one local LookAndFeel object across related lane packs.
  - Treat inherited scalar helpers such as getValueNormalized, setValueNormalized, setValueWithUndo, and updateValueFromProcessorConnection as inactive on this component. Use the slider-pack specific APIs instead.

Complex data chain:

![Slider Pack Data Chain](topology_complex-sliderpack-data-chain.svg)

  - SliderPackProcessor selects the module that owns one or more slider-pack slots.
  - SliderPackData is the complex-data handle for one slot within that module.
  - ScriptSliderPack displays or edits one selected slot in the UI.

  Use the binding properties separately:
  - processorId selects the owning processor.
  - SliderPackIndex selects which slider-pack slot inside that processor should be displayed.

  This is not the normal parameter binding path. parameterId targets processor
  parameters, while slider-pack binding uses SliderPackIndex instead.

Common mistakes:
  - Calling referToData with non-slider-pack complex data -- type mismatch, binding does not switch.
  - Updating setWidthArray with wrong length -- layout falls back and hit-testing assumptions break.
  - Assuming setAllValuesWithUndo respects callback suppression -- current implementation still notifies.
  - Treating setControlCallback second argument as the slider value -- it is the edited index, fetch value with getSliderValueAt(index).
  - Importing many values with callbacks enabled -- avoid callback storms by disabling callbacks during import.

Example:
  const var spk = Content.addSliderPack("Steps", 10, 10);
  spk.setAllValues([1.0, 0.5, 0.0, 0.75]);
  spk.set("stepSequencerMode", true);

Methods (42):
  addToMacroControl                fadeComponent                    get
  getAllProperties                 getChildComponents               getDataAsBuffer
  getGlobalPositionX               getGlobalPositionY               getHeight
  getId                            getLocalBounds                   getNumSliders
  getSliderValueAt                 getValueNormalized               getWidth
  grabFocus                        loseFocus                        referToData
  registerAtParent                 sendRepaintMessage               set
  setAllValueChangeCausesCallback  setAllValues                     setAllValuesWithUndo
  setConsumedKeyPresses            setControlCallback               setKeyPressCallback
  setLocalLookAndFeel              setPosition                      setPropertiesFromJSON
  setSliderAtIndex                 setStyleSheetClass               setStyleSheetProperty
  setStyleSheetPseudoState         setTooltip                       setUsePreallocatedLength
  setValueNormalized               setValueWithUndo                 setWidthArray
  setZLevel                        showControl                      updateValueFromProcessorConnection
