# Diagnostic Ideas

Opportunities for `addDiagnostic()` query lambdas discovered during C++ source analysis.
These are not bugs -- the underlying HISE behavior is correct -- but users would benefit
from early LSP warnings that catch mistakes at parse time.

Sorted by priority (high first).

**Categories:** timeline-dependency, precondition, value-check, combination-check, state-validation
**Priority:** high, medium, low

---

## High

### ErrorHandler -- createErrorHandler without setErrorCallback

- **Category:** timeline-dependency
- **Priority:** high
- **Methods involved:** Engine.createErrorHandler, setErrorCallback
- **Rationale:** Creating an ErrorHandler disables HISE's built-in error overlay (`setUseDefaultOverlay(false)`). If `setErrorCallback()` is never called on the result, system errors become invisible to the user -- no overlay, no callback, no feedback. This is a severe usability trap because the side effect (disabling the default overlay) is immediate and permanent for the session.
- **Sketch:** When `Engine.createErrorHandler()` is called and the result is assigned to a variable, scan subsequent statements in the same scope for `setErrorCallback` called on that variable. If no call is found by end of `onInit`, emit a warning: "ErrorHandler disables the default error overlay. Call setErrorCallback() to handle errors, or errors will be invisible."

## Medium

### Broadcaster.attachToComplexData -- broadcaster arg count mismatch

- **Category:** precondition
- **Priority:** medium
- **Methods involved:** attachToComplexData, attachToComponentMouseEvents, attachToComponentProperties (and other attach methods with fixed arg count requirements)
- **Rationale:** Many Broadcaster attach methods require a specific number of arguments on the broadcaster (e.g., attachToComplexData requires 3, attachToComponentMouseEvents requires 2). A mismatch produces a runtime error but could be caught at parse time by checking the argument count of the `Engine.createBroadcaster()` call that created the broadcaster variable.
- **Sketch:** When an attach method call is encountered, trace the broadcaster variable back to its `Engine.createBroadcaster()` initializer, count the args array length or property count, and compare against the required count for the specific attach method. Emit a warning if they don't match.

### ScriptSlider range helpers -- style must be Range

- **Category:** precondition
- **Priority:** medium
- **Methods involved:** setMinValue, setMaxValue, getMinValue, getMaxValue, contains
- **Rationale:** These methods only work when slider style is `Range`. In other styles they only log an error and return fallback values, so calls often appear to succeed while doing nothing useful.
- **Sketch:** Track `setStyle("Range")` assignments on the same slider variable. When a range-helper method is called without known Range style state, emit a warning.

### ScriptSlider.setMode -- validate mode string literals

- **Category:** value-check
- **Priority:** medium
- **Methods involved:** setMode
- **Rationale:** `setMode` accepts a fixed mode set. Invalid strings silently force internal Linear behavior in current implementation, which is difficult to diagnose from script.
- **Sketch:** When `setMode` is called with a string literal, validate against {"Frequency", "Decibel", "Time", "TempoSync", "Linear", "Discrete", "Pan", "NormalizedPercentage"}. Emit a warning for mismatches.

### ScriptLookAndFeel.registerFunction -- invalid function name string literal

- **Category:** value-check
- **Priority:** medium
- **Methods involved:** registerFunction
- **Rationale:** `registerFunction` accepts any string as the function name and silently stores the function, but only 62 predefined names are ever looked up by the rendering system. A typo means the custom rendering is never invoked and the default rendering appears with no error or warning. The user sees default styling and has no indication that the function name was wrong.
- **Sketch:** When `registerFunction` is called with a string literal as the first argument, validate it against the 62 known function names from `getAllFunctionNames()`. Emit a warning if no match.

### ScriptModulationMatrix.setCurrentlySelectedSource -- requires selectable sources enabled

- **Category:** precondition
- **Priority:** medium
- **Methods involved:** setCurrentlySelectedSource, setSourceSelectionCallback, setMatrixModulationProperties
- **Rationale:** Calling setCurrentlySelectedSource without first enabling selectable sources (via setSourceSelectionCallback or setMatrixModulationProperties with SelectableSources: true) throws a runtime script error. This precondition could be caught at parse time by tracking whether either enabling method was called on the same variable.
- **Sketch:** Track setSourceSelectionCallback and setMatrixModulationProperties calls on ScriptModulationMatrix variables. When setCurrentlySelectedSource is called without either having been called first, emit a warning about the required precondition.

### NeuralNetwork.processFFTSpectrum -- requires loadOnnxModel first

- **Category:** timeline-dependency
- **Priority:** medium
- **Methods involved:** processFFTSpectrum, loadOnnxModel
- **Rationale:** Calling `processFFTSpectrum` without first calling `loadOnnxModel` produces a runtime error ("ONNX model is not loaded. use loadOnnxModel() before calling this method"). This timeline dependency could be caught at parse time by tracking whether `loadOnnxModel` was called on the same NeuralNetwork variable.
- **Sketch:** When `processFFTSpectrum` is called on a NeuralNetwork variable, check if `loadOnnxModel` was previously called on the same variable. Emit a warning if not found.

### NeuralNetwork.loadWeights -- requires build first

- **Category:** timeline-dependency
- **Priority:** medium
- **Methods involved:** loadWeights, build
- **Rationale:** Calling `loadWeights` without first calling `build` (or `loadPytorchModel` which calls build internally) silently fails because the EmptyModel returns `Result::fail("network is not initialised")`, but the scripting wrapper discards the Result. Even once the Result-ignoring bug is fixed, a parse-time check would catch this earlier.
- **Sketch:** When `loadWeights` is called on a NeuralNetwork variable, check if `build` was previously called on the same variable. Emit a warning if not found. Note: `loadPytorchModel`, `loadTensorFlowModel`, and `loadNAMModel` load weights internally, so `loadWeights` after those is also suspicious (redundant for TF/NAM, valid only for Pytorch two-step).

### LorisManager -- analyse before synthesise/process/createEnvelopes/createEnvelopePaths/createSnapshot

- **Category:** timeline-dependency
- **Priority:** medium
- **Methods involved:** analyse, synthesise, process, processCustom, createEnvelopes, createEnvelopePaths, createSnapshot
- **Rationale:** All LorisManager methods except `set` and `get` require a prior `analyse()` call on the same file. Without analysis, these methods either silently return empty results or operate on stale/missing partial data. There is no runtime error message indicating the missing analysis step. A parse-time check could detect when any of these methods is called on a LorisManager variable without a preceding `analyse()` call.
- **Sketch:** When `synthesise`, `process`, `processCustom`, `createEnvelopes`, `createEnvelopePaths`, or `createSnapshot` is called on a LorisManager variable, scan preceding statements in the same scope for an `analyse()` call on the same variable. If none found, emit a warning: "This method requires a prior analyse() call on the same file."

## Low

### Buffer.resample -- invalid interpolationType string literal

- **Category:** value-check
- **Priority:** low
- **Methods involved:** resample
- **Rationale:** `resample` accepts only five interpolation mode strings. Invalid strings throw at runtime with a long error message. A parse-time literal check would catch typos earlier.
- **Sketch:** When `resample` is called with a string literal for `interpolationType`, validate against {"WindowedSinc", "Lagrange", "CatmullRom", "Linear", "ZeroOrderHold"} and warn on mismatch.

### File.toReferenceString -- invalid folderType string

- **Category:** value-check
- **Priority:** low
- **Methods involved:** toReferenceString
- **Rationale:** The `folderType` parameter only accepts 12 specific strings (HISE subdirectory identifiers). An invalid string produces a runtime error "Illegal folder type" but could be caught at parse time. This is the same pattern as `Broadcaster.addComponentRefreshListener`.
- **Sketch:** When `toReferenceString` is called with a string literal, check it against the known set {"AudioFiles", "Images", "SampleMaps", "MidiFiles", "UserPresets", "Samples", "Scripts", "Binaries", "Presets", "XmlPresetBackups", "AdditionalSourceCode", "Documentation"}. Emit a warning if no match.

### Broadcaster.addComponentRefreshListener -- invalid refreshType string

- **Category:** value-check
- **Priority:** low
- **Methods involved:** addComponentRefreshListener
- **Rationale:** The refreshType parameter only accepts 5 specific strings. An invalid string produces a runtime error but could be caught at parse time by checking string literals passed to this parameter against the known valid set.
- **Sketch:** When addComponentRefreshListener is called with a string literal as the second argument, check it against {"repaint", "changed", "updateValueFromProcessorConnection", "loseFocus", "resetValueToDefault"}. Emit a warning if no match.

### Broadcaster.attachToComponentMouseEvents -- invalid callbackLevel string

- **Category:** value-check
- **Priority:** low
- **Methods involved:** attachToComponentMouseEvents
- **Rationale:** The callbackLevel parameter only accepts 6 specific strings. An invalid string produces a runtime error but could be caught at parse time.
- **Sketch:** When attachToComponentMouseEvents is called with a string literal as the second argument, check it against the known callback level strings. Emit a warning if no match.

### Broadcaster.attachToEqEvents -- invalid event type string

- **Category:** value-check
- **Priority:** low
- **Methods involved:** attachToEqEvents
- **Rationale:** The eventTypes parameter only accepts 4 specific strings ("BandAdded", "BandRemoved", "BandSelected", "FFTEnabled"). An invalid string produces a runtime error but could be caught at parse time by checking string literals.
- **Sketch:** When attachToEqEvents is called with a string literal or an array of string literals as the second argument, check each against the known event type set. Emit a warning if no match.

### Graphics.drawMarkdownText -- requires setTextBounds on MarkdownRenderer

- **Category:** timeline-dependency
- **Priority:** low
- **Methods involved:** drawMarkdownText, MarkdownRenderer.setTextBounds
- **Rationale:** `drawMarkdownText` requires `setTextBounds()` to have been called on the MarkdownRenderer object first. Without it, the method fails with a clear error message at runtime, but a parse-time check could catch this earlier by verifying that the MarkdownRenderer variable was seen with a `setTextBounds` call in the same scope.
- **Sketch:** When `drawMarkdownText` is called, trace the MarkdownRenderer variable back to its creation and check if `setTextBounds` was called on it before `drawMarkdownText`. Emit a warning if not.

### Graphics.beginBlendLayer -- invalid blend mode string silently fails

- **Category:** value-check
- **Priority:** low
- **Methods involved:** beginBlendLayer
- **Rationale:** `beginBlendLayer` silently fails when an invalid blend mode string is passed -- the layer is not created and no error is reported. A parse-time check could validate string literals against the 25 known blend mode values and warn about typos or unsupported modes.
- **Sketch:** When `beginBlendLayer` is called with a string literal as the first argument, check it against the known set {"Normal", "Lighten", "Darken", "Multiply", "Average", "Add", "Subtract", "Difference", "Negation", "Screen", "Exclusion", "Overlay", "SoftLight", "HardLight", "ColorDodge", "ColorBurn", "LinearDodge", "LinearBurn", "LinearLight", "VividLight", "PinLight", "HardMix", "Reflect", "Glow", "Phoenix"}. Emit a warning if no match.

### Broadcaster.refreshContextMenuState -- no-op without prior attachToContextMenu

- **Category:** timeline-dependency
- **Priority:** low
- **Methods involved:** refreshContextMenuState, attachToContextMenu
- **Rationale:** `refreshContextMenuState` silently does nothing if `attachToContextMenu` has not been called first. A parse-time check could warn when `refreshContextMenuState` is called on a broadcaster that was never seen to have `attachToContextMenu` called.
- **Sketch:** Track broadcasters that have `attachToContextMenu` called in the same scope. When `refreshContextMenuState` is called, check whether the broadcaster variable was previously seen with `attachToContextMenu`. Emit a warning if not.

### ComplexGroupManager.isNoteNumberMapped -- requires createNoteMap before use

- **Category:** timeline-dependency
- **Priority:** medium
- **Methods involved:** isNoteNumberMapped, createNoteMap
- **Rationale:** `isNoteNumberMapped` requires `createNoteMap()` to have been called first for the same layer. Without it, a script error is thrown at runtime. A parse-time check could catch this common setup mistake earlier by verifying that `createNoteMap` was called on the same ComplexGroupManager variable before any `isNoteNumberMapped` call.
- **Sketch:** When `isNoteNumberMapped` is called on a ComplexGroupManager variable, scan earlier statements in the same scope for `createNoteMap` on the same variable with a matching layer argument. Warn if no prior call is found.

### Synth.attachNote -- requires setFixNoteOnAfterNoteOff before use

- **Category:** timeline-dependency
- **Priority:** medium
- **Methods involved:** attachNote, setFixNoteOnAfterNoteOff
- **Rationale:** `attachNote` requires `setFixNoteOnAfterNoteOff(true)` to have been called first. Without it, a runtime script error is thrown: "You must call setFixNoteOnAfterNoteOff() before calling this method". A parse-time check could catch this by verifying that `setFixNoteOnAfterNoteOff` was called in the same script scope before any `attachNote` call.
- **Sketch:** When `attachNote` is encountered, scan the onInit scope (or the current script's preceding statements) for a `setFixNoteOnAfterNoteOff` call. Emit a warning if not found.

### Sampler.setActiveGroup / setMultiGroupIndex / getRRGroupsForMessage -- requires enableRoundRobin(false)

- **Category:** timeline-dependency
- **Priority:** medium
- **Methods involved:** enableRoundRobin, setActiveGroup, setActiveGroupForEventId, setMultiGroupIndex, setMultiGroupIndexForEventId, getRRGroupsForMessage, refreshRRMap
- **Rationale:** Six methods require `enableRoundRobin(false)` to have been called first. Calling them with RR enabled produces a runtime error, but this is a common setup mistake that could be caught at parse time.
- **Sketch:** When any of `setActiveGroup`, `setActiveGroupForEventId`, `setMultiGroupIndex`, `setMultiGroupIndexForEventId`, `getRRGroupsForMessage`, or `refreshRRMap` is called on a Sampler variable, scan earlier statements in the same scope for `enableRoundRobin(false)` on the same variable. Warn if no prior call is found.

### Sampler.getRRGroupsForMessage -- requires refreshRRMap() first

- **Category:** timeline-dependency
- **Priority:** medium
- **Methods involved:** getRRGroupsForMessage, refreshRRMap
- **Rationale:** `getRRGroupsForMessage` requires `refreshRRMap()` to have been called after loading a sample map. Without it, the internal RR map may be stale or empty, producing incorrect group counts. The error is silent -- no runtime message.
- **Sketch:** When `getRRGroupsForMessage` is called, scan for a preceding `refreshRRMap()` call on the same Sampler variable. Warn if not found.

### Synth.setClockSpeed -- invalid clock speed value

- **Category:** value-check
- **Priority:** low
- **Methods involved:** setClockSpeed
- **Rationale:** `setClockSpeed` only accepts the values 0, 1, 2, 4, 8, 16, 32. Any other value produces a runtime error. A parse-time check could validate integer literals passed to this parameter against the known set and warn about invalid values like 3, 6, or 64.
- **Sketch:** When `setClockSpeed` is called with an integer literal, check it against {0, 1, 2, 4, 8, 16, 32}. Emit a warning if no match.

### Engine.getTextForValue / getValueForText -- invalid converter mode string

- **Category:** value-check
- **Priority:** low
- **Methods involved:** getTextForValue, getValueForText
- **Rationale:** Both methods accept a `converterMode` / `convertedMode` string that must be one of 7 exact values (`"Frequency"`, `"Time"`, `"TempoSync"`, `"Pan"`, `"NormalizedPercentage"`, `"Decibel"`, `"Semitones"`). An invalid mode string silently falls back to plain numeric formatting/parsing with no error or warning, producing unexpected results (e.g., `getTextForValue(440.0, "Hz")` returns `"440"` instead of `"440 Hz"`). A parse-time check on string literals could catch typos like `"Hz"`, `"Freq"`, or `"dB"`.
- **Sketch:** When `getTextForValue` or `getValueForText` is called with a string literal as the mode parameter, check it against the known set {"Frequency", "Time", "TempoSync", "Pan", "NormalizedPercentage", "Decibel", "Semitones"}. Emit a warning if no match.

### ScriptTable.referToData -- unsupported source type is silently ignored

- **Category:** value-check
- **Priority:** low
- **Methods involved:** referToData
- **Rationale:** `referToData` accepts only three source forms (`ScriptTableData`, another compatible complex-data component, or `-1`). Other values are silently ignored, so the previous binding remains active without feedback.
- **Sketch:** When `referToData` is called with a literal argument that is neither `-1` nor an object expression known to be a complex-data handle, emit a warning that the call may be ignored at runtime.

### ScriptTable.registerAtParent -- requires dynamic external-data parent

- **Category:** precondition
- **Priority:** low
- **Methods involved:** registerAtParent
- **Rationale:** `registerAtParent` returns `undefined` when the script processor is not a `ProcessorWithDynamicExternalData`, with no error. Users may assume registration worked and pass an invalid handle into follow-up calls.
- **Sketch:** If `registerAtParent` is called and the result is used without a defined-check guard, emit a warning suggesting validation of the returned handle.

### ScriptTable.setMouseHandlingProperties -- unknown config keys are ignored

- **Category:** value-check
- **Priority:** low
- **Methods involved:** setMouseHandlingProperties
- **Rationale:** The parser accepts a fixed key set. Typos in keys (eg. `snapWitdh`) are silently ignored and defaults stay active.
- **Sketch:** For object literals passed into `setMouseHandlingProperties`, warn on keys outside {syncStartEnd, allowSwap, fixLeftEdge, fixRightEdge, snapWidth, numSteps, midPointSize, dragPointSize, endPointSize, useMouseWheelForCurve, margin, closePath}.

### ScriptTable.setTablePopupFunction -- non-function values silently fallback

- **Category:** value-check
- **Priority:** low
- **Methods involved:** setTablePopupFunction
- **Rationale:** Passing a non-function value does not error - the wrapper silently uses default popup text. This often hides mistakes when callback variables are misspelled or unset.
- **Sketch:** Warn when a literal non-function value is passed as the first argument to `setTablePopupFunction`.

### ScriptSliderPack.setWidthArray -- width-map length should match slider count

- **Category:** state-validation
- **Priority:** low
- **Methods involved:** setWidthArray, getNumSliders, set("sliderAmount", ...)
- **Rationale:** `setWidthArray` expects `numSliders + 1` cumulative breakpoints. Mismatched lengths log an error and fall back to equal-width behavior, which can be easy to miss.
- **Sketch:** When `setWidthArray` is called with an array literal and slider count is statically known from nearby `set("sliderAmount", N)` or literal construction patterns, warn if array length is not `N + 1`.

### ScriptSliderPack.setKeyPressCallback -- requires setConsumedKeyPresses first

- **Category:** timeline-dependency
- **Priority:** low
- **Methods involved:** setKeyPressCallback, setConsumedKeyPresses
- **Rationale:** `setKeyPressCallback` depends on prior key-consumption setup. Without `setConsumedKeyPresses`, callbacks appear registered but never fire for key events, which is easy to misdiagnose.
- **Sketch:** When `setKeyPressCallback` is called on a slider-pack variable, scan earlier statements in the same scope for `setConsumedKeyPresses` on the same variable. Warn if no prior call is found.

### ScriptDynamicContainer.setValueCallback -- requires setData first

- **Category:** timeline-dependency
- **Priority:** low
- **Methods involved:** setValueCallback, setData
- **Rationale:** `setValueCallback` silently does nothing if called before `setData()`. The callback requires the data model's Values tree, which does not exist until `setData()` creates it. No error message or warning is produced -- the callback simply never fires.
- **Sketch:** When `setValueCallback` is called on a dynamic container variable, scan earlier statements in the same scope for `setData` on the same variable. Warn if no prior call is found.

### Timer.startTimer -- missing setTimerCallback

- **Category:** timeline-dependency
- **Priority:** low
- **Methods involved:** startTimer, setTimerCallback
- **Rationale:** Calling `startTimer` without a prior `setTimerCallback` causes the timer to auto-stop on the first tick with no warning. The internal `timerCallback()` checks the WeakCallbackHolder validity and calls `stopTimer()` if invalid. The user sees no error -- the timer just silently stops.
- **Sketch:** When `startTimer` is called on a Timer variable, scan earlier statements in the same scope for `setTimerCallback` on the same variable. Warn if no prior call is found.

### ScriptShader.setUniformData -- reserved uniform name overwrite

- **Category:** value-check
- **Priority:** low
- **Methods involved:** setUniformData
- **Rationale:** When `setUniformData` is called with a string literal matching one of the engine-managed uniform names (`iTime`, `uOffset`, `iResolution`, `uScale`), the value is overwritten by the engine on every render frame. The user's value has no effect and no warning is produced. A parse-time check for these known reserved names would prevent confusion.
- **Sketch:** When `setUniformData` is called with a first argument that is a string literal, check if it matches any of the reserved uniform names. If so, emit a warning that this uniform is managed by the engine and user-set values will be overwritten every frame.

---

### Parameter.setRangeProperty -- invalid range property ID string

- **Category:** value-check
- **Priority:** low
- **Methods involved:** setRangeProperty
- **Rationale:** `setRangeProperty` silently ignores invalid property ID strings. The Parameter object registers `MidPoint` as a constant, but `MidPoint` is not accepted by the validation check (`RangeHelpers::isRangeId`). Users who write `p.setRangeProperty(p.MidPoint, 0.5)` get no error and no effect. A parse-time check on string literals or known constants would catch this.
- **Sketch:** When `setRangeProperty` is called with a string literal or a Parameter constant as the first argument, validate against {"MinValue", "MaxValue", "StepSize", "SkewFactor"}. Specifically flag `p.MidPoint` usage since the constant exists but is not a valid range property ID.

## Entry Template (do not delete)

```
### ClassName.methodName -- short description

- **Category:** timeline-dependency | precondition | value-check | combination-check | state-validation
- **Priority:** high | medium | low
- **Methods involved:** methodA, methodB
- **Rationale:** Why this diagnostic would be valuable for users.
- **Sketch:** Informal description of what the query lambda would check.
```
