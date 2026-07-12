# Content -- Method Workbench

## Progress
- [x] addVisualGuide
- [x] callAfterDelay
- [x] createLocalLookAndFeel
- [x] createMarkdownRenderer
- [x] createPath
- [x] createScreenshot
- [x] createShader
- [x] createSVG
- [x] getAllComponents
- [x] getComponent
- [x] getComponentUnderDrag
- [x] getComponentUnderMouse
- [x] getCurrentTooltip
- [x] getInterfaceSize
- [x] getScreenBounds
- [x] isCtrlDown
- [x] isMouseDown
- [x] makeFrontInterface
- [x] makeFullScreenInterface
- [x] refreshDragImage
- [x] restoreAllControlsFromPreset
- [x] setContentTooltip
- [x] setHeight
- [x] setKeyPressCallback
- [x] setName
- [x] setSuspendTimerCallback
- [x] setToolbarProperties
- [x] setUpdateExistingPosition
- [x] setUseHighResolutionForPanels
- [x] setValuePopupData
- [x] setWidth
- [x] showModalTextInput
- [x] addAudioWaveform
- [x] addButton
- [x] addComboBox
- [x] addDynamicContainer
- [x] addFloatingTile
- [x] addImage
- [x] addKnob
- [x] addLabel
- [x] addMultipageDialog
- [x] addPanel
- [x] addSliderPack
- [x] addTable
- [x] addViewport
- [x] addWebView
- [x] componentExists
- [x] setPropertiesFromJSON
- [x] storeAllControlsAsPreset

## Forced Parameter Types

Content uses DynamicObject::setMethod() for all method registrations, not ADD_API_METHOD_N or ADD_TYPED_API_METHOD_N. Therefore, NO methods have forced parameter types. All type handling is done manually inside NativeFunctionArgs wrappers.

(No forced type table needed -- all methods use untyped wrappers.)
