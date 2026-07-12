# Pipeline Issues

Bugs, design issues, and silent failures discovered during C++ source analysis.
Sorted by severity (critical first).

**Types:** silent-fail, missing-validation, inconsistency, code-smell, ux-issue
**Severity:** critical, high, medium, low

---

## Critical

(No issues yet.)

## High

### Buffer.toCharString -- numChars larger than buffer length can hang execution

- **Type:** missing-validation
- **Severity:** high
- **Location:** VariantBuffer.cpp:~214-221
- **Observed:** `samplesPerChar` is computed with integer division `size / numChars`. When `numChars > size`, this becomes 0. The loop `for (int i = 0; i < size; i += samplesPerChar)` then never increments `i`, resulting in a non-terminating loop.
- **Expected:** Validate `numChars <= size` (or clamp to `size`) before calculating `samplesPerChar`.

### Engine.getPlayHead -- returned object is always empty

- **Type:** silent-fail
- **Severity:** high
- **Location:** MainController.cpp:~1707-1720
- **Observed:** The `hostInfo` DynamicObject is created as empty in the MainController constructor (line 194) and never populated. The entire block of `setProperty()` calls in `MainController::setHostBpm()` (lines 1707-1720) that would populate `bpm`, `timeSigNumerator`, `timeSigDenominator`, `timeInSamples`, `timeInSeconds`, `ppqPosition`, `ppqPositionOfLastBarStart`, `isPlaying`, `isRecording`, `ppqLoopStart`, `ppqLoopEnd`, `isLooping`, etc. is commented out. The `getPlayHead()` method returns this empty object, so all property accesses yield `undefined`.
- **Expected:** Either re-enable the property-population code or deprecate `getPlayHead()` with a `reportScriptError` directing users to `createTransportHandler()`.

### Engine.getSemitonesFromPitchRatio -- returns cents instead of semitones

- **Type:** inconsistency
- **Severity:** high
- **Location:** ScriptingApi.h:~317
- **Observed:** The formula `1200.0 * log2(pitchRatio)` returns cents (1/100th of a semitone), not semitones. For a pitch ratio of 2.0, the method returns 1200.0 instead of 12.0. The inverse method `getPitchRatioFromSemitones` uses `pow(2.0, semiTones / 12.0)` which correctly operates in semitones. These two methods are not true inverses: `getPitchRatioFromSemitones(getSemitonesFromPitchRatio(2.0))` does not return 2.0.
- **Expected:** Change the formula to `12.0 * log2(pitchRatio)` to return semitones, matching the method name and the inverse method's convention.

### FixObjectStack.insert / set -- off-by-one reduces effective capacity by 1

- **Type:** silent-fail
- **Severity:** high
- **Location:** FixLayoutObjects.cpp:~1325 (insert), ~1389 (set)
- **Observed:** `insert()` clamps the position pointer with `jmin<int>(position + 1, numElements - 1)` after writing. When the stack has N-1 elements (position = N-1), the write goes to `items[N-1]` and position stays at N-1 because `jmin(N, N-1) = N-1`. The element is written but never counted by `size()`, making it invisible to `indexOf()`, `contains()`, and iteration. The method returns true despite the data being inaccessible. For a capacity-1 stack, every `insert()` produces an invisible element. `set()` uses `isPositiveAndBelow(position, numElements - 1)` which rejects when `position >= numElements - 1`, so it correctly returns false but the effective capacity is still N-1 instead of N.
- **Expected:** Check capacity before writing: `if (position >= numElements) return false;` then increment without clamping. This makes the full capacity usable and returns false when genuinely full.

### UnorderedStack.setIsEventStack -- NoteNumberAndChannel compares truthiness instead of equality

- **Type:** inconsistency
- **Severity:** high
- **Location:** ScriptingApiObjects.h:~1796 (MCF::equals template)
- **Observed:** The `NoteNumberAndChannel` compare mode uses `e1.getNoteNumber() && e2.getNoteNumber() && e1.getChannel() == e2.getChannel()`. The `&&` checks note number truthiness (non-zero) rather than equality. Note number 0 (C-2) never matches because `0` is falsy. Any two events with non-zero note numbers on the same channel match regardless of actual pitch.
- **Expected:** Change to `e1.getNoteNumber() == e2.getNoteNumber() && e1.getChannel() == e2.getChannel()` for correct note-number equality comparison.

### NeuralNetwork.process -- off-by-one in multi-input size validation

- **Type:** silent-fail
- **Severity:** high
- **Location:** ScriptingApiObjects.cpp:~5795 (Array path), ~5806 (Buffer path)
- **Observed:** The input size validation uses `isPositiveAndBelow(inputBuffer->size, input.size())` which requires `input.size() > inputBuffer->size` (strict greater-than). For a model with N inputs, passing an array or buffer with exactly N elements fails the check silently -- no processing occurs and 0.0 is returned. The user must provide N+1 elements for the check to pass, but the copy loop then writes all N+1 elements to an N-element buffer, causing a buffer overrun.
- **Expected:** Change the check to `input.size() >= inputBuffer->size` (or equivalently `!isPositiveAndBelow(input.size(), inputBuffer->size)`). Also clamp the copy loop to `inputBuffer->size` iterations to prevent overrun when extra elements are provided.

### WavetableController.loadData -- uninitialized channel pointers in array-of-buffers path

- **Type:** missing-validation
- **Severity:** high
- **Location:** ScriptingApiObjects.cpp:~5341-5354
- **Observed:** When loading from an array of Buffers, `float* ptrs[NUM_MAX_CHANNELS]` is stack-allocated without initialization. If any element in the input array is not a valid Buffer (`getBuffer()` returns null), the corresponding `ptrs[i]` entry remains uninitialized. The `AudioSampleBuffer` constructor then reads these garbage pointers, causing undefined behavior.
- **Expected:** Zero-initialize the `ptrs` array (e.g., `float* ptrs[NUM_MAX_CHANNELS] = {};`) or validate all elements before constructing the AudioSampleBuffer.

### WavetableController.loadData -- loopRange silently ignored for invalid format

- **Type:** missing-validation
- **Severity:** high
- **Location:** ScriptingApiObjects.cpp:~5341-5354
- **Observed:** The `loopRange` parameter is silently ignored if it is not an array with exactly 2 elements. No error or warning is produced. Passing a single-element array, an object, or a string results in the loop range being quietly discarded, making it difficult to diagnose incorrect resynthesis behavior.
- **Expected:** Validate that `loopRange` is an array with exactly 2 integer elements and report a script error for invalid formats.

## Medium

### WavetableController.setResynthesisOptions -- RemoveNoise defaults to ReverseOrder value

- **Type:** inconsistency
- **Severity:** medium
- **Location:** WavetableTools.cpp:~85 (fromVar)
- **Observed:** `ResynthesisOptions::fromVar` uses `reverseOrder` as the default value for `RemoveNoise`: `removeNoise = o.getProperty("RemoveNoise", reverseOrder);`. When the user omits `RemoveNoise` from the options JSON, it inherits the current value of `reverseOrder` (default false) instead of the intended default (true).
- **Expected:** Should use `removeNoise` as the default: `removeNoise = o.getProperty("RemoveNoise", removeNoise);`

### WavetableController.saveAsAudioFile -- missing null-synth error check

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~5370
- **Observed:** `saveAsAudioFile` does not check whether the wavetable synth reference is valid before proceeding. All other methods in this class call `reportScriptError("No wavetable synth")` when the synth reference is null, but `saveAsAudioFile` silently returns without error.
- **Expected:** Add the standard `if (synth == nullptr) { reportScriptError("No wavetable synth"); return; }` guard at the top, consistent with the other methods.

### WavetableController.saveAsHwt -- silently ignores empty wavetable data

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~5385
- **Observed:** `saveAsHwt` silently does nothing when no wavetable data is currently loaded in the synth. No error message is produced. Other save/export methods typically report an error when there is no data to export.
- **Expected:** Report a script error when the wavetable data is empty, e.g., "No wavetable data loaded -- call resynthesise() first".

### WavetableController.saveAsHwt -- silently ignores non-ScriptFile parameter

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~5385
- **Observed:** If the parameter is not a ScriptFile object, the method silently does nothing. Unlike `saveAsAudioFile` which uses `getFileFromVar` (accepting both ScriptFile and string paths), `saveAsHwt` only checks for ScriptFile via `dynamic_cast` and silently ignores all other input types.
- **Expected:** Report a script error when the parameter is not a ScriptFile, e.g., "saveAsHwt expects a File object".

### NeuralNetwork.connectToGlobalCables -- method not registered for scripting API

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~5756-5775 (constructor), ~5948-5967 (implementation)
- **Observed:** `connectToGlobalCables` has a full C++ implementation with `String inputId, String outputId` parameters, but is not registered via `ADD_API_METHOD_2` in the constructor and has no `Wrapper` entry. The method cannot be called from HISEScript. The related `CableInputCallback` struct is also implemented but unreachable.
- **Expected:** Add `ADD_API_METHOD_2(connectToGlobalCables)` to the constructor and `API_VOID_METHOD_WRAPPER_2(ScriptNeuralNetwork, connectToGlobalCables)` to the Wrapper struct.

### NeuralNetwork model loaders -- scripting wrappers ignore Result from core methods

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~5870-5999 (build, loadWeights, loadTensorFlowModel, loadPytorchModel, loadNAMModel)
- **Observed:** Five scripting wrappers call core methods that return `Result` but discard the return value. If model building, weight loading, or format parsing fails, the error message from the core (e.g., "network is not initialised", "Can't create layer with ID X") is silently lost. The user sees no error and the model silently remains in a broken state.
- **Expected:** Check the `Result` and call `reportScriptError(result.getErrorMessage())` on failure, consistent with how `loadOnnxModel` already handles its error path.

### NeuralNetwork.createModelJSONFromTextFile -- silently returns empty object for invalid argument

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~5860 (createModelJSONFromTextFile wrapper)
- **Observed:** If the argument is not a valid ScriptFile, the method silently returns an empty object. No error message is shown. The user has no indication that the file was not parsed.
- **Expected:** Report a script error when the argument is not a valid ScriptFile, e.g., "createModelJSONFromTextFile expects a File object".

### NeuralNetwork.loadTensorFlowModel -- missing try-catch around constructor

- **Type:** inconsistency
- **Severity:** medium
- **Location:** hi_neural.cpp:~759-775 (loadTensorFlowModel)
- **Observed:** `NeuralNetwork::loadTensorFlowModel` directly calls `new TensorFlowModel(jsonData)` without a try-catch block. The TensorFlowModel constructor parses JSON via `nlohmann::json::parse` which can throw `std::exception`. In contrast, `build()` and `loadNAMModel()` both wrap their model construction in try-catch blocks that convert exceptions to `Result::fail`.
- **Expected:** Wrap the TensorFlowModel construction in a try-catch block consistent with `build()` and `loadNAMModel()`.

### ScriptModulationMatrix.connect -- always returns false

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptModulationMatrix.cpp:~643-661
- **Observed:** The method signature returns `bool` but the actual connection operation is deferred via `callSuspended()`, which takes a `std::function<void(...)>`. The lambda's bool return value is silently discarded by the implicit conversion. The outer function always returns `false` regardless of whether the connection was successfully added or removed.
- **Expected:** Either change the return type to `void` (since the deferred execution makes the result unavailable at the call site), or restructure to communicate the result back synchronously.

### ScriptModulationMatrix.connect -- silently ignores invalid sourceId

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptModulationMatrix.cpp:~643-661
- **Observed:** When `sourceId` does not match any entry in the source list, the method silently does nothing. No script error is thrown and the return value is always false (see above), so the caller cannot distinguish "invalid source" from "connection failed." Other HISE methods that accept processor/source IDs (e.g., `setCurrentlySelectedSource`) validate the ID and throw a script error when not found.
- **Expected:** Report a script error when `sourceId` is not found in the source list, e.g., "sourceId 'X' was not found in the source list."

### GlobalRoutingManager.connectToOSC -- always returns false

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~9048
- **Observed:** The method unconditionally returns `false` at line 9048. The internal connection success (`ok` variable from `m->connectToOSC(data)` at line 9027) is used to decide whether to register the OSC listener, but is not propagated as the return value.
- **Expected:** Return the `ok` variable to indicate whether the OSC connection was successfully established.

### Node.setParent -- node detached from graph before target validation

- **Type:** inconsistency
- **Severity:** medium
- **Location:** DspNetwork.cpp (ScriptingObjects::ScriptNode::setParent implementation)
- **Observed:** `setParent()` removes the node from its current parent before validating the target container. If the target name is not found or does not resolve to a valid container, the node has already been detached from the graph. In backend builds with undo enabled, this can be reversed, but in frontend builds the node is orphaned with no recovery path.
- **Expected:** Validate the target container before removing the node from its current parent, or make the operation atomic (remove + add in a single undoable action that rolls back on failure).

### DspNetwork.setParameterDataFromJSON -- always returns true instead of ok flag

- **Type:** silent-fail
- **Severity:** medium
- **Location:** DspNetwork.cpp:~991-1020
- **Observed:** The method computes a local `ok` flag that is set to `true` when at least one parameter is matched and updated. However, the method returns `true` unconditionally (line ~1019) instead of returning `ok`. If no node IDs or parameter IDs match, the caller receives `true` despite no parameters being set.
- **Expected:** Return `ok` instead of `true` so the caller can detect when no parameters were matched.

### DspNetwork.undo -- nullptr dereference when undo is disabled

- **Type:** missing-validation
- **Severity:** medium
- **Location:** DspNetwork.cpp:~943-946
- **Observed:** `undo()` calls `getUndoManager(true)->undo()` without null-checking the return value. `getUndoManager()` returns nullptr when `enableUndo` is false (the default in frontend/exported plugin builds). Calling `undo()` in a frontend build dereferences nullptr.
- **Expected:** Check the return value: `if (auto* um = getUndoManager(true)) return um->undo(); return false;`

### ContainerChild.getValue -- missing validity check returns stale data

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp:~6538
- **Observed:** `getValue()` does not call `isValidOrThrow()` before accessing the Values tree. On an invalid reference, it returns stale data without warning. The sibling method `get()` (line ~6320) does check validity via `isValidOrThrow()`.
- **Expected:** Add `isValidOrThrow()` check consistent with `get()` and other accessor methods.

### ContainerChild.setPaintRoutine -- uses isValid instead of isValidOrThrow

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp:~6589
- **Observed:** `setPaintRoutine()` uses `isValid()` instead of `isValidOrThrow()`. On an invalid reference, the call silently does nothing. `setControlCallback()` (line ~6549) and `setChildCallback()` (line ~6603) both use `isValidOrThrow()` and throw a script error on invalid references.
- **Expected:** Use `isValidOrThrow()` for consistency with the other callback registration methods.

### ScriptTable -- crash on save when switching processorId at runtime

- **Type:** ux-issue
- **Severity:** medium
- **Source:** Forum topic 10491
- **Observed:** Binding multiple TableProcessors to a single ScriptTable by switching `processorId` at runtime can cause a crash when saving the project. The crash does not occur during MIDI playback but triggers on save.
- **Expected:** Switching `processorId` at runtime should not destabilise the save path. Either support dynamic rebinding safely or report a script error at the point of the second binding.

### Unlocker.writeExpansionKeyFile -- silently returns false for invalid header prefix

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptExpansion.cpp:~3460
- **Observed:** When `keyData` does not start with `"Expansion List"`, the method silently returns `false` without reporting any error. The return value is indistinguishable from a legitimate write failure (e.g., disk error). The user has no indication that the input data was malformed.
- **Expected:** Report a script error when `keyData` does not start with `"Expansion List"`, e.g., "keyData must start with 'Expansion List'".

### Unlocker.contains -- returns true when unlocker reference is null

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptExpansion.cpp:~3445
- **Observed:** When the weak unlocker reference is null, `contains()` returns `true` instead of `false`. In a copy protection context, a permissive default means that feature-gating checks silently pass when the licensing system is unavailable, potentially granting access to gated features.
- **Expected:** Return `false` when the unlocker reference is null, consistent with other methods like `isUnlocked()` and `canExpire()` which return `false` on null.

### ChildSynth.setEffectChainOrder -- doPoly parameter is ignored

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~4512
- **Observed:** The `doPoly` parameter is accepted in the method signature but hardcoded to `false` when calling `EffectProcessorChain::setFXOrder()`. The call reads `fx->setFXOrder(false, { p.x, p.y }, chainOrder)` instead of `fx->setFXOrder(doPoly, { p.x, p.y }, chainOrder)`. This means only master effect order can be changed, regardless of what the caller passes for `doPoly`.
- **Expected:** Pass the `doPoly` parameter through: `fx->setFXOrder(doPoly, { p.x, p.y }, chainOrder)`.

### Buffer.decompose -- fast threshold array gated by wrong size check

- **Type:** missing-validation
- **Severity:** medium
- **Location:** SiTraNoConverter.cpp:~243
- **Observed:** `ConfigData` parsing checks `if(ft.isArray() && st.size() == 2)` before reading `FastTransientTreshold`. The second condition references `st` instead of `ft`, so valid fast-threshold data is ignored unless `SlowTransientTreshold` also has exactly two elements.
- **Expected:** Validate fast thresholds with `if (ft.isArray() && ft.size() == 2)` so `FastTransientTreshold` is parsed independently.

### Buffer.detectPitch -- negative start offset is not validated

- **Type:** missing-validation
- **Severity:** medium
- **Location:** VariantBuffer.cpp:~281-284
- **Observed:** `startSample` is only upper-clamped via `jmin((int)n.arguments[1], bufferSize - numSamples)`. Negative values pass through, then `PitchDetection::detectPitch` reads `buffer.getSample(..., startSample + i)` with negative indices.
- **Expected:** Clamp `startSample` to a valid range with a lower bound of 0 (or throw a script error for negative indices).

### Buffer.getNextZeroCrossing -- negative index is not validated

- **Type:** missing-validation
- **Severity:** medium
- **Location:** VariantBuffer.cpp:~557-565
- **Observed:** The scan index is cast from script input and used directly in `ptr[i]` and `ptr[i + 1]` without lower-bound clamping. Negative values start the loop below zero and read before the buffer start.
- **Expected:** Clamp the start index to `0..size-2` or reject negative values with a script error.

### Buffer.getPeakRange -- negative start offset is not validated

- **Type:** missing-validation
- **Severity:** medium
- **Location:** VariantBuffer.cpp:~537-541
- **Observed:** The start offset uses `jmin((int)n.arguments[0], bufferSize - numSamples)` with no lower-bound clamp. Negative offsets pass into `findMinMax`, which can read before the buffer.
- **Expected:** Clamp `startSample` to a non-negative range before calling `findMinMax`.

### Buffer.getRMSLevel -- negative start offset is not validated

- **Type:** missing-validation
- **Severity:** medium
- **Location:** VariantBuffer.cpp:~488-491
- **Observed:** `startSample` is upper-clamped with `jmin` but not lower-clamped, so negative offsets can reach `getRMSLevel` and read invalid memory.
- **Expected:** Clamp `startSample` to `0..bufferSize-numSamples` or throw for negative offsets.

### Buffer.getSlice -- negative offset can create invalid slice pointers

- **Type:** missing-validation
- **Severity:** medium
- **Location:** VariantBuffer.cpp:~583-592
- **Observed:** `offsetInBuffer` is clamped with `jmin(numSamples, arg)` but has no lower bound. Negative values pass through and are used in `getWritePointer(0, offset)`, creating an invalid pointer before the buffer start.
- **Expected:** Clamp `offsetInBuffer` to `0..size` (or throw for negatives) before pointer acquisition.

### Buffer.indexOfPeak -- negative start offset is not validated

- **Type:** missing-validation
- **Severity:** medium
- **Location:** VariantBuffer.cpp:~380-384
- **Observed:** `startSample` uses `jmin((int)n.arguments[0], bufferSize - numSamples)` without lower-bound clamping. Negative offsets reach `getReadPointer(0, offset)` and can read out of bounds.
- **Expected:** Clamp `startSample` to a non-negative range before reading.

### Buffer.normalise -- gainInDecibels parameter is ignored

- **Type:** inconsistency
- **Severity:** medium
- **Location:** VariantBuffer.cpp:~184-186
- **Observed:** The optional gain path calls `Decibels::decibelsToGain(gain)` where `gain` is the local variable initialized to `1.0f`, not the script argument. Because the result is then clamped to `0..1`, the computed target gain stays `1.0f`, so any passed dB value is ignored.
- **Expected:** Convert `n.arguments[0]` from dB to linear gain and use that value for normalization.

### Buffer.applyMedianFilter -- missing argument returns undefined without error

- **Type:** missing-validation
- **Severity:** medium
- **Location:** VariantBuffer.cpp:~228-239
- **Observed:** When `windowSize` is omitted, the method exits through `RETURN_IF_NO_THROW(var())` and returns `undefined` instead of reporting invalid usage. This appears to succeed from script but no filtered buffer is produced.
- **Expected:** Reject missing or non-positive `windowSize` with a descriptive script error, or provide an explicit default window size.

### Buffer.decompose -- threshold key spelling mismatch causes silent config drop

- **Type:** inconsistency
- **Severity:** medium
- **Location:** SiTraNoConverter.cpp:~236-247
- **Observed:** The parser only accepts `SlowTransientTreshold` / `FastTransientTreshold` (missing second `h`). Scripts using the conventional spelling `...Threshold` are silently ignored, so user-supplied threshold tuning is dropped without feedback.
- **Expected:** Accept both spellings for backward compatibility, or report a clear error when unknown threshold keys are passed.

### ScriptSlider.setMode -- invalid mode silently forces Linear without property update

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp:~2618-2629
- **Observed:** When `setMode()` receives an invalid mode string, it sets the internal `m` enum to `HiSlider::Mode::Linear` and returns early. It does not report an error and does not update the `mode` property in the ValueTree. Runtime conversion behavior can therefore switch to Linear while `get("mode")` still reports the previous valid mode string.
- **Expected:** Reject invalid mode strings with a descriptive script error (or at minimum keep internal mode unchanged), and keep runtime mode and stored `mode` property in sync.

### ScriptSlider.setStyle -- invalid style string desynchronizes stored property and runtime style

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp:~2735-2764
- **Observed:** `setStyle()` stores the provided style string in the property tree before validating it with `ApiHelpers::getStyleForString<Slider::SliderStyle>()`. For invalid strings, helper conversion returns `SliderStyle::IncDecButtons` as a fallback, which is explicitly treated as invalid by the subsequent `if (s != SliderStyle::IncDecButtons)` guard. The method then skips `styleId` update without reporting an error. Result: the `style` property can show an invalid value while runtime slider behavior keeps the previous style.
- **Expected:** Validate the style string before writing the property, report a script error for invalid styles, and keep stored `style` text synchronized with the active runtime style.

### Engine.getComplexDataReference -- FilterCoefficients accepted but unhandled

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~2546
- **Observed:** The `dataTypes` StringArray includes `"FilterCoefficients"` at index 3, so it passes the `indexOf` validation check. However, the switch statement that constructs the return object has no case for `ExternalData::DataType::FilterCoefficients`. In release builds this silently returns `undefined`. In debug builds it hits `jassertfalse`. The error message also omits `"FilterCoefficients"` from its list of valid types.
- **Expected:** Either add a case for `FilterCoefficients` that returns the appropriate wrapper, or remove `"FilterCoefficients"` from the `dataTypes` array and explicitly reject it with a descriptive error message.

### MarkdownRenderer.setImageProvider -- non-array input silently clears all resolvers

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingGraphics.cpp (MarkdownAction::setImageProvider)
- **Observed:** If the `data` parameter is not an array (e.g., a single object, number, or string), the method silently clears all existing image and link resolvers on the internal renderer and creates no new entries. The user has no indication that all image resolution capability was removed.
- **Expected:** Report a script error when `data` is not an array, e.g., "data must be an Array of image provider entries".

### UserPresetHandler.setUseCustomUserPresetModel -- silently ignores non-function callbacks

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptExpansion.cpp:~250
- **Observed:** If either `loadCallback` or `saveCallback` is not a valid JavaScript function, the method silently returns without enabling the custom data model or reporting any error. The user has no indication that the call had no effect. Subsequent calls to `setCustomAutomation` then fail with the confusing error "you need to enable setUseCustomDataModel() before calling this method", which does not hint at the actual cause (invalid callback arguments).
- **Expected:** Report a script error when either callback is not a function, e.g., "loadCallback must be a function" or "saveCallback must be a function".

### UserPresetHandler.updateAutomationValues -- IndexSorter uses first["id"] for both comparands

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptExpansion.cpp:~591
- **Observed:** In the `IndexSorter::compareElements` method, both `i1` and `i2` are constructed from `first["id"].toString()` instead of `i2` using `second["id"].toString()`. This causes the comparator to always return 0 (equal), effectively disabling the index-based sort. Values are applied in the order they appear in the input array rather than sorted by automation index.
- **Expected:** Line 591 should read `Identifier i2(second["id"].toString());` instead of `Identifier i2(first["id"].toString());`.

### UserPresetHandler.updateAutomationValues -- undo path does not capture old values for array input

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptExpansion.cpp:~472-523
- **Observed:** The `AutomationValueUndoAction` constructor captures old values via `newData.getDynamicObject()`, which returns null when `newData` is an Array (the only valid non-integer input format). The `oldData` member remains uninitialized (`undefined`). When `undo()` calls `updateAutomationValues(oldData, ...)`, the undefined value is neither an int, DynamicObject, nor Array, so the method silently does nothing. `Engine.undo()` after an undoable `updateAutomationValues` call with array data has no effect.
- **Expected:** The undo action constructor should iterate the array of `{"id", "value"}` objects and capture old values in the same array format, or the method should convert array input to DynamicObject format before creating the undo action.

### UserPresetHandler.setParameterGestureCallback -- numExpectedArgs mismatch (2 vs 3)

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptExpansion.cpp:~74 (constructor), ScriptExpansion.cpp:~435 (setter), ScriptExpansion.cpp:~165 (invocation)
- **Observed:** The `parameterGestureCallback` WeakCallbackHolder is initialized with `numExpectedArgs=2` in the constructor (line 74) and re-created with 2 in `setParameterGestureCallback` (line 435). The parse-time diagnostic (`ADD_CALLBACK_DIAGNOSTIC` at line 105) also checks for 2 arguments. However, `onParameterGesture` (line 154) passes 3 arguments (type, slotIndex, startGesture) via `callSync(NativeFunctionArgs)`. Users following the diagnostic guidance write 2-parameter callbacks and silently miss the `startGesture` boolean.
- **Expected:** Change `numExpectedArgs` to 3 in both the constructor initializer and `setParameterGestureCallback`, so the diagnostic correctly reports that the callback expects 3 arguments.

### TransportHandler.setOnTransportChange -- clearIf targets wrong callback slot

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~8452
- **Observed:** The sync branch of `setOnTransportChange` calls `clearIf(tempoChangeCallbackAsync, f)` instead of `clearIf(transportChangeCallbackAsync, f)`. This means registering a synchronous transport callback does not clear a previously registered async transport callback for the same function. It may also inadvertently clear an unrelated tempo async callback if it holds the same function reference.
- **Expected:** Should call `clearIf(transportChangeCallbackAsync, f)` to match the pattern used in all other `setOn*` methods.

### MidiList.setRange -- parameter name does not match loop behavior

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~155, ScriptingApiObjects.h:~296
- **Observed:** The loop `for (int i = startIndex; i < numToFill; i++)` uses the `numToFill` parameter as an absolute end bound, not as a count relative to `startIndex`. Calling `setRange(10, 5, 99)` fills zero slots because `10 < 5` is immediately false. The parameter name `numToFill` implies a relative count.
- **Recommended fix:** Rename the parameter from `numToFill` to `endIndex` (in both the header declaration and Doxygen comment) to match the actual loop behavior. No runtime change needed - the loop logic itself is fine, only the name is misleading. Update the Doxygen comment from "Sets a range of items to the same value" to something like "Sets slots from startIndex up to (but not including) endIndex to the same value."

### MidiList.restoreFromBase64String -- numValues counter not recalculated

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~170
- **Observed:** After `restoreFromBase64String()` overwrites the raw `int[128]` array via `MemoryBlock::fromBase64Encoding` + `memcpy`, the internal `numValues` counter is not recalculated. `isEmpty()` and `getNumSetValues()` return stale values from before the restore until the next `setValue()` call forces a recount.
- **Expected:** Recalculate `numValues` by scanning the array after restoring, or call the existing recount logic that `setValue()` triggers.

### TransportHandler.setEnableGrid -- error message says wrong range

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~8644
- **Observed:** The error message `"Illegal tempo value. Use 1-18"` implies index 0 is invalid, but the code accepts index 0 (Whole note) as a valid tempo factor. The `TempoSyncer::getTempoName(tempoFactor)` call succeeds for index 0.
- **Expected:** Change the error message to `"Illegal tempo value. Use 0-18"` to include the valid Whole note index.

### GlobalCable.connectToGlobalModulator -- silent failure when parent is not GlobalModulatorContainer

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~9613
- **Observed:** If the modulator is found by name but its parent is not a `GlobalModulatorContainer`, the method silently does nothing. The `dynamic_cast<GlobalModulatorContainer*>` fails and execution falls through without reporting an error or returning an indication of failure.
- **Expected:** Report a script error when the modulator exists but its parent is not a `GlobalModulatorContainer` (e.g., "Modulator 'X' must be inside a GlobalModulatorContainer").

### MidiAutomationHandler.setAutomationDataFromObject -- silently clears all automation on non-Array input

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~10093
- **Observed:** If `automationData` is not an Array (e.g., a single object, number, or string), `ValueTreeConverters::convertVarArrayToFlatValueTree` produces an empty ValueTree. `restoreFromValueTree` then clears all existing automation entries and adds nothing. The user's automation data is silently wiped with no error.
- **Expected:** Validate that `automationData` is an Array before proceeding, and report a script error if not, e.g., "automationData must be an Array".

### MacroHandler.setMacroDataFromObject -- silently ignores non-Array input

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~9841
- **Observed:** If `jsonData` is not an Array, the method silently returns without modifying any macro connections or firing the update callback. No error is reported.
- **Expected:** Report a script error when `jsonData` is not an Array, e.g., "jsonData must be an Array".

### MidiAutomationHandler.setUpdateCallback -- silently ignores non-function argument

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~10098
- **Observed:** If the argument is not a valid JavaScript function, the method silently returns without modifying or clearing the callback. No error is reported.
- **Expected:** Report a script error when the callback argument is not a function, e.g., "callback must be a function".

### MidiAutomationHandler.setUpdateCallback -- no way to clear the callback

- **Type:** ux-issue
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~10096-10111
- **Observed:** There is no mechanism to unregister the update callback. Passing a non-function value is silently ignored (see above), so the previous callback remains active. The callback persists until the MidiAutomationHandler object is garbage collected.
- **Expected:** Either accept `false` to clear the callback, or provide a separate `clearUpdateCallback()` method.

### MacroHandler.setUpdateCallback -- silently ignores non-function argument

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~9862
- **Observed:** If the argument is not a valid JavaScript function, the method silently returns without modifying or clearing the callback. No error is reported.
- **Expected:** Report a script error when the callback argument is not a function, e.g., "callback must be a function".

### MacroHandler.setUpdateCallback -- no way to clear the callback

- **Type:** ux-issue
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~9860-9871
- **Observed:** There is no mechanism to unregister the update callback. Passing a non-function value is silently ignored (see above), so the previous callback remains active. The callback persists until the MacroHandler object is garbage collected.
- **Expected:** Either accept `false`/`undefined` to clear the callback, or provide a separate `clearUpdateCallback()` method.

### MacroHandler.getMacroDataObject -- getCallbackArg iterates all parameters, overwrites range on single object

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~10032-10055
- **Observed:** `getCallbackArg(macroIndex, p, parameterIndex, wasAdded)` receives a specific `parameterIndex` but then loops over ALL parameters in the macro slot (`md->getNumParameters()`). Each loop iteration overwrites the range properties (`FullStart`, `FullEnd`, `Start`, `End`, `Interval`, `Skew`, `Inverted`) and potentially the `Attribute` and `CustomAutomation` properties on the same DynamicObject. When a macro slot has multiple connected parameters, each returned object contains the range data of the LAST parameter in the slot rather than the specific parameter it represents. The `parameterIndex` argument is only used to set the initial `Attribute` at line 10026 but gets overwritten if any parameter is a custom automation.
- **Expected:** The loop should either be removed (just look up the specific parameter by `parameterIndex`) or the function should build range data from only the parameter matching `parameterIndex`.

### Message.getPolyAfterTouchNoteNumber -- accesses mutable pointer in const getter

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~768
- **Observed:** `getPolyAfterTouchNoteNumber()` is a const getter but accesses `messageHolder->getAfterTouchNumber()` (the mutable pointer) instead of `constMessageHolder->getAfterTouchNumber()`. In read-only contexts (e.g., voice start modulators), `constMessageHolder` is set but `messageHolder` is null. The null check on `constMessageHolder` passes, then the method dereferences the null `messageHolder`, causing undefined behavior.
- **Expected:** Should access `constMessageHolder->getAfterTouchNumber()` instead of `messageHolder->getAfterTouchNumber()`.

### Message.getPolyAfterTouchPressureValue -- accesses mutable pointer in const getter

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~781
- **Observed:** Same root cause as `getPolyAfterTouchNoteNumber`. `getPolyAfterTouchPressureValue()` is a const getter but accesses `messageHolder->getAfterTouchValue()` instead of `constMessageHolder->getAfterTouchValue()`. Dereferences null pointer in read-only contexts.
- **Expected:** Should access `constMessageHolder->getAfterTouchValue()` instead of `messageHolder->getAfterTouchValue()`.

### Message.getProgramChangeNumber -- wrong pointer check and wrong error message

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~648
- **Observed:** The null-pointer guard checks `messageHolder` (mutable pointer) instead of `constMessageHolder`, and the error message says `"setVelocity()"` instead of `"getProgramChangeNumber()"`. This means the method incorrectly fails in read-only contexts where `constMessageHolder` is set but `messageHolder` is null, and produces a misleading error message.
- **Expected:** Should check `constMessageHolder == nullptr` and report `"getProgramChangeNumber()"` in the error message.

### Message.setStartOffset -- null check on wrong pointer

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~985
- **Observed:** The `ENABLE_SCRIPTING_SAFE_CHECKS` guard checks `constMessageHolder == nullptr` for the null check, but the actual write on line 996 uses `messageHolder->setStartOffset(...)`. In read-only contexts (voice start modulators), `constMessageHolder` is non-null but `messageHolder` is null. The null check passes, then the write dereferences null `messageHolder`.
- **Expected:** Should check `messageHolder == nullptr` (the mutable pointer) to match the write target, consistent with all other setter methods (setNoteNumber, setVelocity, setChannel, etc.).

### Message.sendToMidiOut -- missing null-pointer guard on messageHolder

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~1105-1121
- **Observed:** `sendToMidiOut()` has no `ENABLE_SCRIPTING_SAFE_CHECKS` guard around `messageHolder`. When called outside a mutable callback context, `makeArtificial()` silently returns 0 (since `makeArtificialInternal` checks `if (messageHolder != nullptr)`), but the next line `getScriptProcessor()->getMainController_()->sendToMidiOut(*messageHolder)` dereferences the null `messageHolder` pointer, causing undefined behavior (crash).
- **Expected:** Add a null-pointer check on `messageHolder` at the top of `sendToMidiOut()` with `reportIllegalCall("sendToMidiOut()", "midi event")`, matching the pattern used by other mutable-context methods like `ignoreEvent()`.

### Message.setStartOffset -- error message off by one

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~985
- **Observed:** The error message says "Max start offset is 65536 (2^16)" but the check `newStartOffset > UINT16_MAX` correctly accepts 65535 and rejects 65536. The actual maximum accepted value is 65535 (UINT16_MAX), not 65536 as the message states.
- **Expected:** Change the error message to "Max start offset is 65535 (UINT16_MAX)" or "Max start offset is 65535 (2^16 - 1)".

### Message.store -- silently ignores invalid MessageHolder argument

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~1062
- **Observed:** If the argument is not a valid MessageHolder object, `dynamic_cast<ScriptingObjects::ScriptingMessageHolder*>` returns null and the method silently does nothing. No error is reported. The user has no indication that the store operation failed.
- **Expected:** Report a script error when the argument is not a MessageHolder, e.g., "store() expects a MessageHolder object created by Engine.createMessageHolder()".

### MessageHolder.addToTimestamp -- int16 truncation silently corrupts delta values

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~5654
- **Observed:** The MessageHolder wrapper casts the `deltaSamples` parameter to `int16` before passing to `HiseEvent::addToTimeStamp(int)`. Values outside -32768..32767 silently overflow. For example, passing 50000 wraps to -15536, shifting the timestamp backward instead of forward. No error or warning is produced.
- **Expected:** Either accept the full `int` range (matching the underlying `HiseEvent::addToTimeStamp(int)` signature which takes an `int`), or validate the range and report a script error when the delta exceeds the int16 range.

### Date.ISO8601ToMilliseconds -- silently returns 0 for invalid ISO-8601 strings

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~3770
- **Observed:** If an invalid or unparseable string is passed, `juce::Time::fromISO8601` returns a default-constructed Time (epoch = 0). The method returns 0, which is indistinguishable from a genuinely parsed Unix epoch timestamp. No error or warning is reported.
- **Expected:** Validate the input string before parsing, or check if the result is the default Time when the input is not an empty string, and report a script error for unparseable input.

### FileSystem.browse / browseForDirectory -- startFolder does not accept string paths

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~7527-7548
- **Observed:** `browse` and `browseForDirectory` only check `startFolder.isInt()` and `dynamic_cast<ScriptFile*>`, silently ignoring absolute path strings. The multi-select variants (`browseForMultipleDirectories`, `browseForMultipleFiles`) use `getFileFromVar` which also handles strings. Passing a string path to the single-select methods silently results in an empty start directory.
- **Expected:** Use `getFileFromVar` (or equivalent string-path resolution) in `browse` and `browseForDirectory` for consistency with the multi-select variants.

### FileSystem.encryptWithRSA -- no key validation before RSA operation

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~7706
- **Observed:** `encryptWithRSA` constructs an `RSAKey` from the provided string and applies it unconditionally without checking `RSAKey::isValid()`. A malformed key string produces garbage hex output without any error. `decryptWithRSA` (line 7722) does validate the key.
- **Expected:** Check `RSAKey::isValid()` before applying the key, consistent with `decryptWithRSA`. Return an empty string or report a script error if the key is invalid.

### FileSystem.getFolder -- no range check on locationType integer

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~7457
- **Observed:** The `locationType` var is cast directly to `SpecialLocations` via `(SpecialLocations)(int)locationType` with no range validation. Passing an integer outside 0-11 feeds an out-of-range value into the `getFileStatic` switch statement, resulting in undefined C++ behavior.
- **Expected:** Validate that the integer is in range 0-11 before the cast, and report a script error for out-of-range values, e.g., "Invalid SpecialLocations value. Use FileSystem.AudioFiles (0) through FileSystem.Music (11)".

### FileSystem.getBytesFreeOnVolume -- returns 0 silently for invalid folder arguments

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~7581
- **Observed:** If the `folder` parameter is not a `SpecialLocations` constant or `File` object (e.g., a string path), the internal `juce::File` remains default-constructed and `getBytesFreeOnVolume()` returns 0. The result is indistinguishable from a genuinely full volume. No error or warning is produced.
- **Expected:** Use `getFileFromVar` for consistent var-to-File resolution, or report a script error when the argument is neither an integer constant nor a File object.

### File.copyDirectory -- reportScriptError does not prevent execution from continuing

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~810
- **Observed:** When the target File object exists but is not a directory, `reportScriptError("target is not a directory")` is called at line 811, but execution falls through to `f.copyDirectoryTo(sf->f)` at line 813 because there is no `return` or `RETURN_IF_NO_THROW` after the error. In non-throwing builds, the copy operation proceeds with a non-directory target.
- **Expected:** Add a `return false;` or `RETURN_IF_NO_THROW(false);` after the `reportScriptError` call at line 811 to prevent the copy from proceeding.

### File.toReferenceString -- DspNetworks folder type cannot match due to trailing slash mismatch

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~375, PresetHandler.h:~SubDirectories enum
- **Observed:** The method auto-appends `/` to the `folderType` parameter if it does not end with `/`. However, the `FileHandlerBase::getIdentifier()` for `DspNetworks` returns `"DspNetworks"` (no trailing slash) -- the only subdirectory without one. After appending, the comparison becomes `"DspNetworks/" == "DspNetworks"` which fails. Passing `"DspNetworks"` always produces the "Illegal folder type" script error.
- **Expected:** Either add a trailing slash to the DspNetworks identifier in `FileHandlerBase::getIdentifier()` for consistency, or skip the auto-append when the input already matches without a slash.

### File.createDirectory -- silent failure when directory creation fails

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~820
- **Observed:** The return value of `juce::File::createDirectory()` is not checked. If directory creation fails (e.g., insufficient permissions, invalid characters in name, disk full), the method silently returns a File handle to the non-existent child path without reporting any error. The user has no indication that the directory was not created.
- **Expected:** Check the `juce::Result` returned by `createDirectory()` and report a script error if it failed, or return `undefined` instead of a File handle to a non-existent path.

### File.loadAsMidiFile -- silently rejects .midi and .smf file extensions

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~430
- **Observed:** The method checks `f.getFileExtension() == ".mid"` and silently returns an empty value for any other extension. Standard MIDI file extensions `.midi` and `.smf` are silently rejected without error. The user has no indication that the file was not loaded.
- **Expected:** Either accept `.midi` and `.smf` as valid extensions in addition to `.mid`, or report a script error when a non-`.mid` file is passed (e.g., "loadAsMidiFile only supports .mid files").

### File.writeMidiFile -- silently returns false for non-Array eventList

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~490
- **Observed:** If `eventList` is not an Array (e.g., a single MessageHolder, a number, or a string), the method silently returns `false` without reporting any error. The return value `false` is indistinguishable from a legitimate write failure (e.g., permission denied).
- **Expected:** Report a script error when `eventList` is not an Array, e.g., "eventList must be an Array of MessageHolder objects".

### File.writeMidiFile -- non-MessageHolder array elements silently skipped

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~495
- **Observed:** When the event array contains non-MessageHolder elements (e.g., plain objects, numbers, strings), they are silently skipped. If all elements are invalid, an empty MIDI file is written to disk without any error or warning.
- **Expected:** Either report a script error when a non-MessageHolder element is encountered, or at minimum report a warning when no valid events were found and the resulting MIDI file is empty.

### File.writeAudioFile -- existing file deleted before write, lost on failure

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~540
- **Observed:** The method calls `f.deleteFile()` before creating the `AudioFormatWriter`. If the writer creation subsequently fails (e.g., unsupported bit depth for the format, unrecognized extension), the original file has already been deleted and its data is lost. The method returns `false` but the data cannot be recovered.
- **Expected:** Write to a temporary file first, then replace the original atomically (similar to how `writeString` uses `replaceWithText`). Alternatively, defer deletion until after the writer is successfully created.

### Colours.fromHsl -- alpha element cast truncates fractional float values

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~7340
- **Observed:** `fromHsl` casts the alpha element as `(uint8)(int)hsl[3]`. When `toHsl` outputs alpha as a 0.0-1.0 float (e.g., 0.5 for half transparency), `(int)0.5` truncates to 0, producing a fully transparent colour. The `toHsl`/`fromHsl` roundtrip is broken for any fractional alpha value between 0 and 1. Only `0.0` and values >= `1.0` survive.
- **Expected:** Cast alpha consistently with the other float elements: `(float)hsl[3]` instead of `(uint8)(int)hsl[3]`, since JUCE's `Colour::fromHSL` takes `float alpha` in the 0.0-1.0 range.

### Download.getProgress -- double-counts existingBytesBeforeResuming on resumed downloads

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~1302-1311
- **Observed:** `getProgress()` reads `data.numDownloaded` and `data.numTotal` (which already include `existingBytesBeforeResuming`, added in the `progress()` callback at lines 1437-1438), then adds `existingBytesBeforeResuming` a second time (lines 1304-1305). For fresh downloads the offset is 0 so the result is correct. For resumed downloads the numerator and denominator are both inflated by the same extra offset, so the ratio is slightly off (shifted toward 1.0 for small files, negligible for large files). The intermediate `d` and `t` values are incorrect regardless.
- **Expected:** Either remove the `+ existingBytesBeforeResuming` additions in `getProgress()` (since the `data` properties already include them), or read from `bytesDownloaded_` and `totalLength_` directly (like `getDownloadSize()` and `getNumBytesDownloaded()` do).

### Synth.stopTimer -- null pointer dereference when parentMidiProcessor is null

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~5801
- **Observed:** In non-deferred mode, `stopTimer()` has `if(parentMidiProcessor != nullptr) owner->stopSynthTimer(...)` on line 5799, but `parentMidiProcessor->setIndexInChain(-1)` on line 5801 is outside the null guard. If `parentMidiProcessor` is null (called from a non-MIDI processor context), `stopSynthTimer` is correctly skipped but `setIndexInChain(-1)` dereferences null, causing undefined behavior.
- **Expected:** Move `parentMidiProcessor->setIndexInChain(-1)` inside the `if(parentMidiProcessor != nullptr)` block, or add a separate null guard before it.

### Synth.getWavetableController -- error message says "routing matrix" instead of wavetable

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApi.cpp:~6300
- **Observed:** The error message when the processor is not a `WavetableSynth` says `"[id] does not have a routing matrix"` instead of a wavetable-related message. This is a copy-paste error from `getRoutingMatrix` immediately above it.
- **Expected:** Change the error message to `"[id] is not a WavetableSynth"` or `"[id] does not have wavetable capabilities"`.

### Synth.getWavetableController -- missing null-check for processor lookup

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApi.cpp:~6295
- **Observed:** Unlike `getMidiPlayer` and `getRoutingMatrix` which check `if (p == nullptr)` separately before the type cast, `getWavetableController` skips the null check and goes directly to `dynamic_cast<WavetableSynth*>(p)`. When the processor name does not exist, the error message is the wrong "routing matrix" message rather than "[id] was not found".
- **Expected:** Add `if (p == nullptr) reportScriptError(processorId + " was not found");` before the `dynamic_cast` check, matching the pattern in `getMidiPlayer` and `getRoutingMatrix`.

### Synth.getChildSynthByIndex -- silent failure on non-Chain owner or out-of-bounds index

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~5989
- **Observed:** When the parent synth is not a Chain type (e.g., a single ModulatorSynth without children), or when the index is out of bounds, the method silently returns a ScriptingSynth wrapper with a null processor pointer. No script error is reported. Calling methods on the returned handle produces confusing errors unrelated to the original lookup failure.
- **Expected:** Report a script error when the owner is not a Chain (matching `getNumChildSynths`'s "can only be called on Chains!" pattern) and when the index is out of bounds (e.g., "index X is out of range (0-N)").

### Synth.getChildSynthByIndex -- reportIllegalCall error message says "getChildSynth()"

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApi.cpp:~6005
- **Observed:** The `reportIllegalCall` message when `getChildSynthByIndex` is called outside `onInit` says `"getChildSynth()"` instead of `"getChildSynthByIndex()"`. Copy-paste error from the name-based variant.
- **Expected:** Change to `reportIllegalCall("getChildSynthByIndex()", "onInit")`.

### Synth.getDisplayBufferSource -- reportIllegalCall error message says "getScriptingTableProcessor()"

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApi.cpp:~6195
- **Observed:** The `reportIllegalCall` message when `getDisplayBufferSource` is called outside `onInit` says `"getScriptingTableProcessor()"` instead of `"getDisplayBufferSource()"`. Copy-paste error from the table processor method.
- **Expected:** Change to `reportIllegalCall("getDisplayBufferSource()", "onInit")`.

### Synth.getIdList -- silently returns undefined when called outside onInit

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApi.cpp:~6030
- **Observed:** When called outside `onInit` (i.e., `objectsCanBeCreated()` returns false), `getIdList` silently returns `var()` (undefined) instead of calling `reportIllegalCall`. Other `get*()` methods like `getModulator`, `getEffect`, `getChildSynth` report a clear "can only be called in onInit" error message.
- **Expected:** Add `reportIllegalCall("getIdList()", "onInit")` in the else branch, consistent with other `get*()` methods.

### Synth.getAllEffects -- missing reportIllegalCall when called outside onInit

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~6066
- **Observed:** `getAllEffects` checks `objectsCanBeCreated()` but does not call `reportIllegalCall("getAllEffects()", "onInit")` when the check fails. It silently falls through to `RETURN_IF_NO_THROW({})`, returning an empty value instead of an array. Other `get*()` methods like `getModulator`, `getEffect`, `getTableProcessor` report a clear error message via `reportIllegalCall`. Users calling `getAllEffects` outside `onInit` get no feedback.
- **Expected:** Add `reportIllegalCall("getAllEffects()", "onInit")` in the else branch, consistent with other `get*()` methods.

### Synth.getAudioSampleProcessor -- missing objectsCanBeCreated() guard

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~6092
- **Observed:** `getAudioSampleProcessor` does not check `objectsCanBeCreated()` to restrict calls to `onInit`, unlike `getTableProcessor`, `getSliderPackProcessor`, `getModulator`, `getEffect`, and most other `get*()` methods. It allocates a `ScriptAudioSampleProcessor` wrapper on the heap, which is unsafe outside `onInit`. The `WARN_IF_AUDIO_THREAD` guard only triggers in debug builds.
- **Expected:** Add an `objectsCanBeCreated()` check with `reportIllegalCall("getAudioSampleProcessor()", "onInit")` to match the pattern used by other `get*()` methods.

### Synth.getTableProcessor -- reportIllegalCall error message says "getScriptingTableProcessor()"

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApi.cpp:~6138
- **Observed:** The `reportIllegalCall` message when `getTableProcessor` is called outside `onInit` says `"getScriptingTableProcessor()"` instead of `"getTableProcessor()"`. The internal C++ method name leaks into the user-facing error message.
- **Expected:** Change to `reportIllegalCall("getTableProcessor()", "onInit")`.

### Synth.getSampler -- reportIllegalCall error message says "getScriptingAudioSampleProcessor()"

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApi.cpp:~6222
- **Observed:** The `reportIllegalCall` message when `getSampler` is called outside `onInit` says `"getScriptingAudioSampleProcessor()"` instead of `"getSampler()"`. Copy-paste error from the audio sample processor method.
- **Expected:** Change to `reportIllegalCall("getSampler()", "onInit")`.

### Engine.createFixObjectFactory -- non-object layoutDescription silently produces non-functional factory

- **Type:** missing-validation
- **Severity:** medium
- **Location:** FixLayoutObjects.cpp:~279-301
- **Observed:** If `layoutDescription` is not a DynamicObject (e.g., an array, string, number, or undefined), `createLayout()` produces an empty `MemoryLayoutItem::List` and sets `initResult = Result::fail("No data")`. The Factory is still created and returned to the user. All subsequent `create()`, `createArray()`, and `createStack()` calls check `initResult.wasOk()`, fail silently, and return empty `var()`. The user receives no error message explaining why the factory produces no objects.
- **Expected:** Report a script error in the Factory constructor when `initResult` fails, e.g., `reportScriptError("Invalid layout description: " + initResult.getErrorMessage())`.

### ScriptWebView.addBufferToWebSocket -- silently ignores non-Buffer argument

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp (ScriptWebView::addBufferToWebSocket)
- **Observed:** If the second argument is not a Buffer (e.g., an Array, number, or string), the method silently does nothing. No error is reported and no buffer is registered at the given index.
- **Expected:** Report a script error when the second argument is not a Buffer, e.g., "addBufferToWebSocket expects a Buffer as the second argument".

### ScriptAudioWaveform.referToData -- silently ignores invalid data source argument

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptComponentWrappers.cpp (ComplexDataScriptComponent::referToData implementation)
- **Observed:** If the `audioData` argument is not a ScriptAudioFile, another ComplexDataScriptComponent, nor the integer `-1`, the method silently does nothing -- no error is reported and the data source remains unchanged. The user has no indication that the call had no effect.
- **Expected:** Report a script error when the argument is not one of the three accepted types, e.g., "referToData expects a ScriptAudioFile, another waveform component, or -1 to reset".

### ScriptTable.referToData -- silently ignores unsupported argument types

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp:~3198-3224
- **Observed:** `referToData()` delegates to `ComplexDataScriptComponent::referToDataBase`, which only handles ScriptComplexDataReferenceBase objects, ComplexDataScriptComponent objects, or integer `-1`. Any other argument type is silently ignored without error, so the table keeps its previous data binding.
- **Expected:** Report a script error when `tableData` is not one of the accepted argument types.

### ScriptSliderPack.referToData -- silently ignores unsupported argument types

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp:~3793-3796, ScriptComponentWrappers.cpp (ComplexDataScriptComponent::referToDataBase)
- **Observed:** `referToData()` delegates to `ComplexDataScriptComponent::referToDataBase`, which only handles ScriptComplexDataReferenceBase objects, ComplexDataScriptComponent objects, or integer `-1`. Any other argument type is silently ignored without error, so the slider pack keeps its previous data binding.
- **Expected:** Report a script error when `sliderPackData` is not one of the accepted argument types.

### ScriptTable.getTableValue -- missing table binding silently returns 0.0

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp:~3256-3274
- **Observed:** If `getTableValue()` cannot resolve a valid `SampleLookupTable` from the current binding, it returns `0.0` without reporting an error. Calls appear to succeed but return a misleading value that is indistinguishable from a valid table lookup at zero.
- **Expected:** Report a script error (or return `undefined`) when no valid lookup table is available, so missing bindings are diagnosable.

### ScriptTable.setTablePoint -- invalid point index silently ignored

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp:~3229-3254
- **Observed:** Out-of-range `pointIndex` values are ignored without any error or warning, so the call appears to succeed but does not modify the table.
- **Expected:** Validate the index and report a script error when it does not reference an existing point.

### ScriptTable.setSnapValues -- validation occurs after internal state mutation

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp:~3302-3322
- **Observed:** Non-array input reports an error in wrapper-side validation, but the method has already mutated internal `snapValues` state. This yields a partial-failure state where script output reports an error yet behavior still changes.
- **Expected:** Validate input type before mutating internal state, or roll back state changes when validation fails.

### ScriptTable.registerAtParent -- unsupported parent type returns undefined silently

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp:~3324-3350
- **Observed:** When the script processor is not a `ProcessorWithDynamicExternalData`, `registerAtParent()` returns `undefined` without reporting an error. Users can proceed with an invalid handle without feedback on why registration failed.
- **Expected:** Report a script error when registration is not possible in the current parent context.

### ScriptSliderPack.registerAtParent -- unsupported parent type returns undefined silently

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp:~3812-3815
- **Observed:** `registerAtParent()` returns the result of `registerComplexDataObjectAtParent(index)`. When the script processor is not a `ProcessorWithDynamicExternalData`, this returns `undefined` without reporting an error. Users can continue with an invalid handle without knowing registration failed.
- **Expected:** Report a script error when registration is not possible in the current parent context.

### ScriptSliderPack.setAllValuesWithUndo -- callback toggle is ignored

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp:~3668-3670
- **Observed:** Notification mode uses `auto n = (true || allValueChangeCausesCallback) ? sendNotificationAsync : dontSendNotification;`, so the `allValueChangeCausesCallback` setting is effectively ignored and callbacks always fire.
- **Expected:** Respect the `allValueChangeCausesCallback` flag, or rename the API to clarify that undo writes always notify by design.

### ScriptDynamicContainer.setValueCallback -- silently does nothing if called before setData()

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp (ScriptDynamicContainer::Wrapper::setValueCallback)
- **Observed:** If `setValueCallback` is called before `setData()`, the method silently returns without registering the callback or reporting any error. The data model's Values tree does not exist yet, so the ValueTree listener cannot be attached. The user has no indication that the callback will never fire.
- **Expected:** Report a script error when `setData()` has not been called yet, e.g., "setValueCallback requires setData() to have been called first".

### TableProcessor.addTablePoint -- x and y values not clamped to 0.0-1.0 unlike setTablePoint

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (ScriptingTableProcessor::addTablePoint)
- **Observed:** `addTablePoint` stores x and y values as-is without clamping to the 0.0-1.0 range. In contrast, `setTablePoint` clamps all coordinate and curve values to 0.0-1.0 via `jlimit`. Out-of-range values in `addTablePoint` are written directly to the graph point array, producing unexpected table shapes.
- **Expected:** Clamp x and y to 0.0-1.0 before storing, consistent with `setTablePoint`.

### TableProcessor.setTablePoint -- out-of-range pointIndex silently ignored

- **Type:** missing-validation
- **Severity:** medium
- **Location:** Tables.cpp:~122
- **Observed:** `Table::setTablePoint` checks `if (pointIndex >= 0 && pointIndex < graphPoints.size())` and silently skips the modification when the index is out of range. The `ScriptingTableProcessor::setTablePoint` wrapper does not add any additional validation. The call appears to succeed but has no effect, and the user has no way to diagnose the issue. Same root cause as the ScriptTable.setTablePoint issue (both delegate to `Table::setTablePoint`).
- **Expected:** Report a script error when `pointIndex` is out of range, e.g., "pointIndex N is out of range (0-M)".

### AudioSampleProcessor.getAudioFile -- slotIndex parameter is not bounds-checked

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (ScriptAudioSampleProcessor::getAudioFile)
- **Observed:** The `slotIndex` parameter is passed directly to `getComplexBaseType` without validation. AudioSampleProcessor modules always have exactly one audio file slot (index 0). Passing any other index creates an AudioFile handle pointing to a non-existent slot, which may produce undefined behavior when reading or writing through the handle.
- **Expected:** Validate that `slotIndex` is 0 (or within `getNumDataObjects(ExternalData::DataType::AudioFile)`) and report a script error for out-of-range values, e.g., "slotIndex must be 0 for AudioSampleProcessor (single audio file slot)".

### SliderPackData.getStepSize -- method not registered in constructor

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~2236-2259 (constructor), ~2261 (implementation)
- **Observed:** `getStepSize()` is declared in the header (line 1322), has a C++ implementation (line 2261), and appears in the API reference via Doxygen, but is NOT registered via `ADD_API_METHOD_0(getStepSize)` in the constructor and has no Wrapper entry. The method cannot be called from HiseScript. The step size can be set via `setRange()` but there is no way to read it back from script.
- **Expected:** Add `ADD_API_METHOD_0(getStepSize)` to the constructor and add the corresponding `API_METHOD_WRAPPER_0` to the Wrapper struct, or remove the method from the API reference if it is intentionally not exposed.

### AudioFile.loadBuffer -- uninitialized channel pointers and size mismatch with Array input

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~1720-1733
- **Observed:** When `bufferData` is an Array, the loop only sets `ptrs[i]` when `getBuffer()` succeeds. If any array element is not a valid Buffer, `ptrs[i]` remains uninitialized (stack garbage), and `AudioSampleBuffer ab(ptrs, numChannels, numSamples)` uses the garbage pointer, causing a crash or data corruption. Additionally, `numSamples` is overwritten each iteration with the last valid Buffer's length. If Buffers have different sizes, shorter ones are read past their end.
- **Expected:** Validate that all array elements are valid Buffers with identical sample counts before constructing the AudioSampleBuffer. Report a script error or skip invalid elements.

### FixObjectStack.set -- scripting wrapper discards bool return value

- **Type:** inconsistency
- **Severity:** medium
- **Location:** FixLayoutObjects.cpp:~1278 (Wrapper struct)
- **Observed:** The `set()` method returns `bool` in C++ (false when the stack is full and the object is not found), but the Wrapper struct uses `API_VOID_METHOD_WRAPPER_1` instead of `API_METHOD_WRAPPER_1`. The scripting layer wraps it as a void method, so the return value is always `undefined` from HISEScript. Users cannot detect when `set()` fails due to a full stack.
- **Expected:** Change `API_VOID_METHOD_WRAPPER_1(Stack, set)` to `API_METHOD_WRAPPER_1(Stack, set)` so the bool return is preserved.

### FixObjectStack.toBase64 / fromBase64 -- position pointer not included in serialization

- **Type:** inconsistency
- **Severity:** medium
- **Location:** FixLayoutObjects.cpp:~1048-1066 (inherited from Array)
- **Observed:** `toBase64()` and `fromBase64()` are inherited from FixObjectArray and serialize only the raw memory block. The Stack's `position` pointer (which determines `size()`) is not included. After a `fromBase64()` restore, `size()` returns the pre-restore value, not the original stack's used count. A stack serialized with 5 elements and restored into an empty stack has `size() == 0` despite all 5 elements' data being present in memory.
- **Expected:** Override `toBase64()` and `fromBase64()` in Stack to include the position value in the serialized data, or document that users must save and restore `size()` separately.

### FixObjectArray.indexOf / contains -- silently returns -1/0 for non-FixObject arguments

- **Type:** missing-validation
- **Severity:** medium
- **Location:** FixLayoutObjects.cpp (indexOf implementation)
- **Observed:** Passing a non-FixObject value (e.g., a plain JSON object `{"id": 5}`) to `indexOf` silently returns -1. `contains` delegates to `indexOf` and silently returns 0. No error is reported. The user has no indication that the argument type was wrong.
- **Expected:** Report a script error when the argument is not a FixObject from the same factory layout.

### FixObjectArray.copy -- silently returns 0 for non-Buffer/Array target

- **Type:** missing-validation
- **Severity:** medium
- **Location:** FixLayoutObjects.cpp (copy implementation)
- **Observed:** Passing a target that is neither a Buffer nor an Array silently returns 0 with no error message. The return value 0 is indistinguishable from a legitimate failure.
- **Expected:** Report a script error when the target is not a Buffer or Array, e.g., "copy target must be a Buffer or Array".

### FixObjectArray.fill -- plain JSON object silently resets all elements instead of filling

- **Type:** missing-validation
- **Severity:** medium
- **Location:** FixLayoutObjects.cpp (fill implementation)
- **Observed:** Passing a plain JSON object (e.g., `{"id": 5}`) does not fill elements with those values. The dynamic_cast to ObjectReference fails, triggering the clear branch. All elements are reset to defaults with no error or warning. The silent wrong-branch behavior is indistinguishable from an intentional clear.
- **Expected:** Report a script error when a non-FixObject, non-undefined value is passed, to distinguish intentional clearing from accidental wrong-type input.

### UnorderedStack.setIsEventStack -- EqualData constant exposed but unimplemented

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.h:~1800 (MCF::equals template, default case)
- **Observed:** The `EqualData` constant (value 4) is exposed via `addConstant()` but has no case in the MCF compare template. It falls through to the `default` branch which hits `jassertfalse` (debug assertion) and returns `false`. In release builds, all comparisons silently return false, making `contains()`, `remove()`, and `removeIfEqual()` non-functional.
- **Expected:** Either implement the EqualData comparison logic or remove the constant from the scripting API to prevent users from selecting a broken mode.

### ComplexGroupManager.setGroupVolume -- silently ignored on non-Custom layers

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (setGroupVolume -> ComplexGroupManager::setGroupVolume -> CustomLayer cast)
- **Observed:** `setGroupVolume()` internally casts the target layer to `CustomLayer*`. On non-Custom LogicType layers, this cast returns null and the gain value is silently discarded. No script error or console warning is produced.
- **Expected:** Validate the layer's LogicType before the cast. If not Custom, call `reportScriptError()` with a message indicating that `setGroupVolume` only works on Custom layers.

### ChildSynth.addGlobalModulator -- silent return on invalid globalMod argument

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~4470
- **Observed:** If `globalMod` is not a ScriptingModulator (fails the `dynamic_cast<ScriptingModulator*>`), the method silently returns undefined without reporting any error. The user has no indication that the argument type was wrong.
- **Expected:** Report a script error when the `globalMod` argument is not a ScriptingModulator, e.g., "globalMod must be a Modulator reference from a GlobalModulatorContainer".

### ChildSynth.addModulator -- silent return on invalid type name

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~4480
- **Observed:** If `typeName` does not match any known modulator C++ class name, `ModuleHandler::addModule` fails silently and returns undefined. No error message indicates that the type name was invalid.
- **Expected:** Report a script error when the type name does not match a known modulator type, e.g., "Unknown modulator type: [typeName]".

### ChildSynth.addStaticGlobalModulator -- silent return on invalid timeVariantMod argument

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~4475
- **Observed:** If `timeVariantMod` is not a ScriptingModulator (fails the `dynamic_cast<ScriptingModulator*>`), the method silently returns undefined without reporting any error. Same root cause as `addGlobalModulator`.
- **Expected:** Report a script error when the `timeVariantMod` argument is not a ScriptingModulator.

### ChildSynth.asSampler -- creates Sampler wrapping nullptr on invalid object

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~4530
- **Observed:** If the underlying ChildSynth object reference is invalid (synth was deleted or null), `asSampler()` still creates a `ScriptingSampler` wrapping nullptr rather than returning undefined. The user receives a Sampler handle that will crash or produce confusing errors on subsequent calls.
- **Expected:** Check object validity before creating the Sampler wrapper. Return undefined if the synth reference is invalid.

### ChildSynth.getChildSynthByIndex -- silent invalid return on non-Chain type

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~4500
- **Observed:** If the wrapped synth is not a Chain type (SynthGroup or SynthChain), the `dynamic_cast<Chain*>` fails and the method returns an invalid ChildSynth wrapping nullptr without any error message. Subsequent calls on the returned handle produce confusing errors unrelated to the original lookup failure.
- **Expected:** Report a script error when the synth is not a Chain type, e.g., "getChildSynthByIndex can only be called on Chain types (SynthGroup or SynthChain)".

### ChildSynth.getModulatorChain -- no validation of chain index type

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~4490
- **Observed:** Passing an invalid chain index (e.g., 0 for MidiProcessor) may succeed the `dynamic_cast<Modulator*>` but give a handle to the wrong chain type. The error message only triggers when the cast fails completely. No validation checks that the requested index corresponds to a ModulatorChain.
- **Expected:** Validate that the chain index corresponds to a ModulatorChain (indices 1 or 2) before the cast, or at minimum verify the returned processor type is a ModulatorChain.

### Effect.addModulator -- silently returns undefined for invalid type name

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~3600
- **Observed:** If `typeName` does not match any known modulator C++ class name, `ModuleHandler::addModule` fails silently and the method returns `undefined`. No error message indicates that the type name was invalid. Same root cause as `ChildSynth.addModulator`.
- **Expected:** Report a script error when the type name does not match a known modulator type, e.g., "Unknown modulator type: [typeName]".

### Effect.addGlobalModulator / addStaticGlobalModulator -- silently returns undefined for non-Modulator input

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~3624, ~3650
- **Observed:** Both `addGlobalModulator` and `addStaticGlobalModulator` use `dynamic_cast<ScriptingModulator*>(globalMod.getObject())` to validate the second parameter. If the cast fails (e.g., a plain object, string, or wrong scripting API handle is passed), the method silently returns `undefined` without reporting any error. The user has no indication that the call had no effect.
- **Expected:** Report a script error when the second parameter is not a valid Modulator handle, e.g., "globalMod must be a Modulator reference".

### Effect.restoreState -- does not fire attribute change notifications

- **Type:** missing-notification
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (restoreState implementation)
- **Observed:** `restoreState()` restores all parameter values from a Base64 string but does not fire attribute change notifications. Broadcasters attached via `attachToModuleParameter()` do not fire after a `restoreState()` call, leaving connected listeners out of sync with the restored state.
- **Expected:** Either fire attribute change notifications for all restored parameters, or document the limitation prominently. Current workaround: manually re-set each parameter via `fx.setAttribute(fx.Param, fx.getAttribute(fx.Param))` to trigger the notification chain.

### SlotFX.setEffect -- invalid effect name silently clears slot

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~3740-3755
- **Observed:** When `setEffect()` is called with an effect name not in the module list, the C++ `SlotFX::setEffect()` clears the slot and returns `false`. However, the scripting wrapper (`ScriptingSlotFX::setEffect`) ignores this return value and unconditionally wraps `getCurrentEffect()` -- returning an `Effect` handle to EmptyFX. The caller receives a valid-looking Effect object with no indication that the requested effect was not found.
- **Expected:** Check the return value of `slot->setEffect()`. If false, call `reportScriptError()` with the invalid name, or at minimum return an undefined/null value instead of wrapping EmptyFX.

### Modulator.getGlobalModulatorId -- returns empty string silently for non-global modulators

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~2960
- **Observed:** When called on a modulator whose type name does not start with "Global", the method returns an empty string without reporting any error. Other type-restricted methods on Modulator (e.g., connectToGlobalModulator) report a script error for incompatible modulator types. The silent empty-string return is inconsistent and indistinguishable from a valid but unnamed global modulator.
- **Expected:** Report a script error when the modulator is not a global type, e.g., "getGlobalModulatorId() only works on global receiver modulators", consistent with connectToGlobalModulator.

### Modulator.setMatrixProperties -- silently does nothing for non-MatrixModulator types

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~2950
- **Observed:** When called on a modulator that is not a MatrixModulator instance, the dynamic_cast fails and the method silently does nothing. No error is reported. Other type-restricted methods on Modulator (e.g., connectToGlobalModulator, exportScriptControls) report script errors for incompatible modulator types.
- **Expected:** Report a script error when the modulator is not a MatrixModulator, e.g., "setMatrixProperties() only works on MatrixModulator instances".

### Modulator bracket-read -- getAssignedValue always returns 1.0

- **Type:** code-smell
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~2972-2975
- **Observed:** `getAssignedValue()` (the bracket-read operator implementation from AssignableObject) is hardcoded to `return 1.0; // Todo...` regardless of the parameter index. Reading a parameter via bracket syntax `var x = mod["Frequency"]` always returns 1.0 instead of the actual attribute value. The bracket-write operator correctly delegates to `setAttribute()`, so the asymmetry is unexpected.
- **Expected:** Should delegate to `mod->getAttribute(index)` to return the actual attribute value, symmetric with the bracket-write operator.

### ExpansionHandler.getMetaDataFromPackage -- silently returns undefined for non-File argument

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptExpansion.cpp:~1360-1368
- **Observed:** If the `packageFile` argument is not a File object (e.g., a string path, number, or undefined), the `dynamic_cast<ScriptingObjects::ScriptFile*>` fails and the method silently returns `undefined` (`{}`). No error is reported. Other methods with the same pattern (`encodeWithCredentials`, `getExpansionForInstallPackage`, `installExpansionFromPackage`) correctly call `reportScriptError("argument is not a file")`.
- **Expected:** Add `reportScriptError("argument is not a file")` in an `else` branch, consistent with the other File-parameter methods.

### ExpansionHandler.getUninitialisedExpansions -- method not registered in constructor

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptExpansion.cpp:~1142-1178 (constructor), ~1278 (implementation)
- **Observed:** `getUninitialisedExpansions()` is declared in the header (ScriptExpansion.h:247), has a C++ implementation (line 1278), and appears in the Doxygen API reference, but is NOT registered via `ADD_API_METHOD_0(getUninitialisedExpansions)` in the constructor and has no Wrapper entry (`API_METHOD_WRAPPER_0`). The method cannot be called from HiseScript.
- **Expected:** Add `ADD_API_METHOD_0(getUninitialisedExpansions)` to the constructor and add the corresponding `API_METHOD_WRAPPER_0(ScriptExpansionHandler, getUninitialisedExpansions)` to the Wrapper struct.

### Expansion.writeDataFile -- written file invisible to loadDataFile on non-FileBased expansions

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptExpansion.cpp:~1802 (writeDataFile), ~1768 (loadDataFile)
- **Observed:** `writeDataFile` always writes to the filesystem AdditionalSourceCode directory regardless of expansion type. However, `loadDataFile` on Intermediate/Encrypted expansions reads from the embedded data pool using a wildcard reference, not from the filesystem. A write-then-read cycle does not roundtrip: `writeDataFile("config.json", data)` followed by `loadDataFile("config.json")` returns the old pool data, not the just-written data. The written file exists on disk but is invisible to the pool-based load path.
- **Expected:** Either `writeDataFile` should update the pool data for non-FileBased expansions, or `loadDataFile` should check the filesystem first (falling back to the pool), or the asymmetry should be reported as a script error/warning.

### Expansion.getSampleFolder / writeDataFile / setSampleFolder -- missing objectExists() null guard

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptExpansion.cpp:~1726 (getSampleFolder), ~1802 (writeDataFile), ~1733 (setSampleFolder)
- **Observed:** Three methods access the `exp` WeakReference without checking `objectExists()` or `exp != nullptr` first. If the expansion has been unloaded (WeakReference becomes null), these methods dereference a null pointer instead of throwing a descriptive script error. All other methods in the class either check `objectExists()` (getSampleMapList, getImageList, getAudioFileList, getMidiFileList, getDataFileList, getUserPresetList, getProperties, getRootFolder, getExpansionType, getWildcardReference, loadDataFile) or check `exp != nullptr` (setAllowDuplicateSamples, unloadExpansion).
- **Expected:** Add `if (!objectExists()) { reportScriptError("Expansion was deleted"); RETURN_IF_NO_THROW({}); }` at the top of getSampleFolder, writeDataFile, and setSampleFolder, matching the pattern used by getRootFolder and the list methods.

### Array scoped functions -- incorrect return values on empty arrays

- **Type:** inconsistency
- **Severity:** medium
- **Location:** JavascriptEngineObjects.cpp:~636 (callForEach totalReturnValue initialization)
- **Observed:** `every`, `some`, `findIndex`, `map`, and `filter` return `undefined` on empty arrays. The root cause is that `callForEach` initializes `totalReturnValue` as `var()` (undefined), and the per-method lambdas only set it during iteration -- which never executes on empty arrays. This causes: `every` returns `undefined` instead of `true` (vacuous truth); `some` returns `undefined` instead of `false`; `findIndex` returns `undefined` instead of `-1`; `map` and `filter` return `undefined` instead of an empty array. For `map`/`filter`, chaining on the result (e.g. `.length`) throws because undefined has no properties.
- **Expected:** Each scoped function should initialize its return value before the iteration loop: `every` -> `true`, `some` -> `false`, `findIndex` -> `-1`, `map`/`filter` -> empty array. Alternatively, `callForEach` could accept an initial value parameter.

### Builder.get -- silent failure for invalid buildIndex and type mismatch

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~10401-10424
- **Observed:** `get()` returns undefined without error if `buildIndex` is out of range, the module reference has been released, or if `interfaceType` does not match the module's actual C++ type. Other Builder methods (`clearChildren`, `getExisting`, `setAttributes`) report script errors for invalid build indexes via `reportScriptError`.
- **Expected:** Should report a script error for invalid buildIndex (consistent with other Builder methods) and report an error for unrecognized or mismatched interfaceType.

### Builder.clearChildren -- no self-preservation check for calling script processor

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (Builder::clearChildren implementation)
- **Observed:** Unlike `clear()`, which explicitly preserves the calling script processor during demolition, `clearChildren()` has no self-preservation check. If the calling script processor is in the targeted chain (e.g., clearing the MIDI chain that contains the calling script), it will be removed, potentially causing undefined behavior or a silent crash.
- **Expected:** Add a self-preservation check matching `clear()`'s pattern -- skip the calling script processor when iterating the chain's children for removal, or report a script error if the target chain contains the caller.

### Builder.connectToScript -- silent failure on non-JavascriptProcessor target

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (Builder::connectToScript implementation)
- **Observed:** If the module at `buildIndex` is not a JavascriptProcessor (e.g., it is a synth or effect without scripting capability), the method silently does nothing. The C++ `dynamic_cast<JavascriptProcessor*>` returns null, the internal `setConnectedFile` call is skipped, the bool return value (false) is discarded by the void wrapper, and no error is reported.
- **Expected:** Report a script error when the target module is not a JavascriptProcessor, e.g., "Module at buildIndex N is not a script processor".

### Builder.setAttributes -- non-numeric values silently become 0.0

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (Builder::setAttributes implementation)
- **Observed:** All attribute values in the JSON object are cast to `float` via the C++ `(float)` cast. Non-numeric values (strings, objects, arrays, booleans) silently become 0.0 with no validation or warning. The user has no indication that their attribute value was discarded.
- **Expected:** Validate that each value is numeric before casting. Report a script error for non-numeric values, e.g., "Attribute 'Name' must be a numeric value, got [type]".

### ScriptLookAndFeel.registerFunction -- invalid function names silently accepted

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp (ScriptedLookAndFeel::registerFunction)
- **Observed:** `registerFunction` accepts any string as the function name and stores the callback internally, but only 62 predefined names are ever looked up by the rendering code. Invalid names are silently ignored -- the function is stored but never invoked. No error or warning is produced.
- **Expected:** Validate the function name against the 62 predefined names from `getAllFunctionNames()` and report a script error for unrecognized names, e.g., "Unknown LAF function name: [name]".

### ScriptLookAndFeel.registerFunction -- non-function second argument silently ignored

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiContent.cpp (ScriptedLookAndFeel::registerFunction)
- **Observed:** If the second argument is not a valid JavaScript function, the call silently does nothing. No error is thrown and no callback is registered.
- **Expected:** Report a script error when the second argument is not a function, e.g., "paintFunction must be a function".

### ScriptShader.fromBase64 -- silently ignores invalid base64 input

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (ScriptShader::fromBase64)
- **Observed:** If the base64 string is invalid or cannot be decoded, the method silently does nothing -- no shader is compiled and no error is reported. The user has no indication that the load failed.
- **Expected:** Report a script error when the base64 string cannot be decoded, e.g., "Invalid base64 shader data".

### ScriptShader.setFragmentShader -- silently fails in frontend builds for missing shader file

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (ScriptShader::setFragmentShader)
- **Observed:** In frontend (exported plugin) builds, if the shader file name does not match an embedded file in the script collection, the shader silently fails to compile with no error message. The shader area renders blank with no feedback.
- **Expected:** Report a script error or console warning when the shader file is not found in the embedded script collection.

### ScriptShader.setUniformData -- arrays with 1 or more than 4 elements silently ignored

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (ScriptShader::setUniformData)
- **Observed:** Arrays with 1 element or more than 4 elements are silently ignored -- only sizes 2, 3, and 4 are mapped to vec2/vec3/vec4. No error or warning is reported for unsupported array sizes.
- **Expected:** Report a script error when the array size is not 2, 3, or 4, e.g., "Uniform array must have 2-4 elements for vec2/vec3/vec4".

### Parameter.setRangeProperty -- MidPoint constant silently ignored as range property ID

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (ScriptNode::Parameter::setRangeProperty)
- **Observed:** The Parameter class registers `MidPoint` as a constant via `addConstant()`, but `setRangeProperty()` only accepts `MinValue`, `MaxValue`, `StepSize`, and `SkewFactor` as valid IDs (validated by `RangeHelpers::isRangeId`). Passing `p.MidPoint` silently does nothing -- no error, no effect. The constant's existence implies it should be usable with `setRangeProperty()`.
- **Expected:** Either accept `MidPoint` as a valid range property ID in `setRangeProperty()` (converting it to the appropriate skew factor internally), or remove the `MidPoint` constant from Parameter to prevent confusion, or report a script error when `MidPoint` is passed: "MidPoint is not a valid range property ID; use SkewFactor instead".

### Connection.getConnectionType -- returns uninitialized data

- **Type:** code-smell
- **Severity:** medium
- **Location:** NodeBase.h:~713 (class declaration), NodeBase.cpp:~1632 (constructor), NodeBase.cpp:~1795 (method)
- **Observed:** The `ConnectionSource type` member is declared but never assigned anywhere -- not in the constructor, not in any method, and not by any external code that creates ConnectionBase objects. `getConnectionType()` returns `(int)type`, which is indeterminate. The `ConnectionSource` enum (MacroParameter=0, SingleOutputModulation=1, MultiOutputModulation=2) exists but is never used at runtime.
- **Expected:** Determine the connection source type in the constructor based on the ValueTree hierarchy (e.g., parent is `Connections` under a Parameter = MacroParameter, parent is `ModulationTargets` = SingleOutputModulation, parent is under `SwitchTargets` = MultiOutputModulation) and assign `type` accordingly.

### ThreadSafeStorage.storeWithCopy -- string values stored as empty string

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~8248
- **Observed:** The string-copy branch calls `copy.toString()` on a default-constructed (undefined) local variable instead of `dataToStore.toString()`. This produces an empty string regardless of input, so `storeWithCopy("hello")` silently stores `""`.
- **Expected:** The line should read `copy = var(dataToStore.toString());` to correctly copy the input string.

### Settings.stopPerfettoTracing -- file extension validated after trace session ends

- **Type:** ux-issue
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~2827-2852
- **Observed:** `stopPerfettoTracing` calls `mp.endSession(true)` (writing the trace data to the file) before checking whether the file has a `.pftrace` extension. If the extension is wrong, the trace data has already been written to the misnamed file, and then a `reportScriptError` fires. The data is not lost but Perfetto tools may not recognize a file without the `.pftrace` extension.
- **Expected:** Validate the file extension before calling `mp.endSession(true)`, so no trace data is written to an incorrectly named file.

### Settings.setDiskMode -- no range validation on mode parameter

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~2890-2894
- **Observed:** The `mode` parameter is stored directly in `driver->diskMode` and cast to `MainController::SampleManager::DiskMode` without validation. The DiskMode enum only defines values 0 (SSD) and 1 (HDD). Values >= 2 are cast to an out-of-range enum value and passed to `setDiskMode()`, producing undefined streaming behavior.
- **Expected:** Validate that `mode` is 0 or 1 before applying. Report a script error for out-of-range values, e.g., "Invalid disk mode. Use 0 (SSD) or 1 (HDD)".

### Settings.setSampleFolder -- silently does nothing for non-File arguments

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~2870
- **Observed:** If the argument is not a `File` object or if the path is not a directory, the method silently does nothing. No error is reported. The user has no indication that the sample folder was not changed.
- **Expected:** Report a script error when the argument is not a valid File object pointing to a directory, e.g., "setSampleFolder expects a File object pointing to an existing directory".

### Settings.setVoiceMultiplier -- no validation on voice multiplier value

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~2900
- **Observed:** The `newVoiceAmount` parameter is stored directly without range checking. Negative or zero values are accepted silently, potentially producing zero or negative voice counts when multiplied with the base voice count.
- **Expected:** Validate that `newVoiceAmount` is a positive integer (>= 1) and report a script error for invalid values, e.g., "Voice multiplier must be a positive integer".

### Settings.getAvailableSampleRates -- returns strings instead of numbers

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~2810
- **Observed:** `getAvailableSampleRates` returns an array of strings (e.g., `"44100"`, `"48000"`), while `getAvailableBufferSizes` returns an array of integers. This inconsistency means users must call `parseInt()` on sample rates but not on buffer sizes. The difference comes from how the underlying JUCE API is wrapped -- buffer sizes are added as integers while sample rates are added as strings.
- **Expected:** Return integers (or doubles) for consistency with `getAvailableBufferSizes`, or document the string return type prominently.

### ErrorHandler.setErrorCallback -- silently ignores non-function argument

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (ScriptErrorHandler::setErrorCallback)
- **Observed:** If the argument is not a valid JavaScript function (e.g., `false`, a number, a string, or `undefined`), the method silently does nothing. No error is reported and the previous callback (if any) remains active. The user has no indication that the callback registration failed.
- **Expected:** Report a script error when the argument is not a function, e.g., "errorCallback must be a function".

### ErrorHandler.setErrorCallback -- no way to clear the callback

- **Type:** ux-issue
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (ScriptErrorHandler::setErrorCallback)
- **Observed:** There is no mechanism to unregister the error callback. Passing a non-function value is silently ignored (see above), so the previous callback remains active. The callback persists until the ErrorHandler object is garbage collected.
- **Expected:** Either accept `false` to clear the callback, or provide a separate `clearErrorCallback()` method.

### DisplayBufferSource.getDisplayBuffer -- expired weak reference silently returns invalid object

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp (ScriptDisplayBufferSource::getDisplayBuffer)
- **Observed:** If the source processor has been deleted (weak reference expired), the method silently returns an invalid DisplayBuffer object without reporting an error. Subsequent calls on the returned object will fail with unrelated errors.
- **Expected:** Check the weak reference before constructing the DisplayBuffer wrapper. If expired, call `reportScriptError("Source processor was deleted")` or return `undefined`.

### DisplayBuffer.createPath -- sourceRange endSample parameter ignored

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~1913
- **Observed:** Line 1913 reads `auto maxSize = hToUse = getRingBuffer()->getReadBuffer().getNumSamples();` which overwrites `hToUse` (previously set from `src.getHeight()` on line 1907). The subsequent `if (hToUse == -1)` check is dead code. The sample range always spans `[startSample, bufferSize]` regardless of the endSample value passed in sourceRange.
- **Expected:** Separate the assignments: `auto maxSize = getRingBuffer()->getReadBuffer().getNumSamples();` without overwriting `hToUse`, so the user-provided endSample from `src.getHeight()` is respected (with -1 as the sentinel for "use full buffer").

### DisplayBuffer.getResizedBuffer -- resampleMode parameter is unused

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~1848-1891
- **Observed:** The `resampleMode` parameter is declared in the method signature but never referenced in the implementation. Both resampling branches (point sampling for stride < 2.0, min/max midpoint for stride >= 2.0) execute based solely on the computed stride value. The parameter has no effect on behavior.
- **Expected:** Either implement resample mode selection (e.g., switching between point sampling, linear interpolation, min/max) based on the parameter value, or remove the parameter from the API.

### DisplayBuffer.getResizedBuffer -- exact size match returns shared reference instead of copy

- **Type:** inconsistency
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~1848-1891
- **Observed:** When `numDestSamples` matches the read buffer size exactly, the method delegates to `getReadBuffer()`, which returns a direct memory reference to the ring buffer's internal read buffer. For all other sizes, an independent buffer is allocated and returned. The caller cannot predict whether the returned buffer is safe to modify without knowing the exact size relationship.
- **Expected:** Always return an independent copy regardless of size match, or document the shared-reference behavior and provide a separate method for callers who want a copy at the same size.

## Low

### ExpansionHandler.setErrorFunction -- numExpectedArgs mismatch (1 vs 2)

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptExpansion.cpp:~1207-1213 (setter), ~1437-1446 (invocation)
- **Observed:** `setErrorFunction` creates the `WeakCallbackHolder` with `numExpectedArgs=1` (line 1210), but `logMessage` calls the callback with 2 arguments (line 1445): `(message, isCritical)`. The `ADD_CALLBACK_DIAGNOSTIC(errorFunction, setErrorFunction, 0)` at line 1153 uses the WeakCallbackHolder's numExpectedArgs for the parse-time diagnostic check, so if the diagnostic were to enforce argument count, users would be told to write 1-parameter callbacks when 2 parameters are actually passed.
- **Expected:** Change line 1210 to `WeakCallbackHolder(getScriptProcessor(), this, newErrorFunction, 2)` to match the 2 arguments passed in `logMessage`.

### ChildSynth.getRoutingMatrix -- missing checkValidObject guard

- **Type:** missing-validation
- **Severity:** low
- **Location:** ScriptingApiObjects.cpp:~4534
- **Observed:** `getRoutingMatrix()` creates a `ScriptRoutingMatrix` wrapping `synth.get()` without first calling `checkValidObject()`. If the synth reference is invalid (null or deleted), this creates a RoutingMatrix wrapping nullptr. The user gets no error from `getRoutingMatrix()` itself; subsequent calls on the returned matrix will fail with unrelated error messages.
- **Expected:** Add `if (checkValidObject())` guard before creating the ScriptRoutingMatrix, consistent with all other methods on this class.

### Synth.removeModulator -- audio-thread error message says "Effects" instead of "Modules"

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApiObjects.cpp:~6064
- **Observed:** The `ModuleHandler::removeModule` throw says "Effects can't be removed from the audio thread!" regardless of whether the processor being removed is an effect or a modulator. When called via `Synth.removeModulator`, the error message incorrectly mentions "Effects".
- **Expected:** Change the error message to "Modules can't be removed from the audio thread!" or make the message generic (e.g., "Processors can't be removed from the audio thread!").

### Synth.internalAddNoteOn -- start offset error message off by one

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApi.cpp:~6391
- **Observed:** The error message says "Max start offset is 65536 (2^16)" but the check `startOffset > UINT16_MAX` correctly accepts 65535 and rejects 65536. The actual maximum accepted value is 65535 (UINT16_MAX), not 65536 as the message states. Same root cause as `Message.setStartOffset`.
- **Expected:** Change the error message to "Max start offset is 65535 (UINT16_MAX)" or "Max start offset is 65535 (2^16 - 1)".

### Synth.noteOff -- deprecation error message has typo

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApi.cpp:~5485
- **Observed:** The deprecation error message says `"noteOff is deprecated. Use noteOfByEventId instead"` -- "noteOfByEventId" is missing the second 'f'. The correct method name is `noteOffByEventId`.
- **Expected:** Change the error message to `"noteOff is deprecated. Use noteOffByEventId instead"`.

### Path.createStrokedPath -- invalid EndCapStyle/JointStyle string produces undefined enum cast

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~7021-7022
- **Observed:** `ApiHelpers::createPathStrokeType` uses `StringArray::indexOf()` to map `EndCapStyle` and `JointStyle` strings to enum values. If the string does not match any valid value, `indexOf` returns -1, which is cast to the `PathStrokeType::EndCapStyle` or `PathStrokeType::JointStyle` enum. The -1 value is outside the valid enum range (0-2 for both), producing undefined rendering behavior. No error is reported.
- **Expected:** Validate the string against the known values and either default to a safe value (e.g., `butt`/`mitered`) with a warning, or report a script error for invalid strings.

### Path.cubicTo -- coordinates not sanitized against NaN/Inf

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingGraphics.cpp:~1108-1111
- **Observed:** `cubicTo` passes all six coordinate values directly to JUCE's `Path::cubicTo` without the `SANITIZED()` macro applied. Other path methods like `startNewSubPath`, `lineTo`, `addArc`, and `addPieSegment` sanitize their coordinate inputs. Passing NaN/Inf values to `cubicTo` corrupts the path geometry silently.
- **Expected:** Apply `SANITIZED()` to all coordinate values, consistent with `startNewSubPath` and `lineTo`. Note: `quadraticTo` has the same issue.

### Path.quadraticTo -- coordinates not sanitized against NaN/Inf

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingGraphics.cpp:~1101-1104
- **Observed:** `quadraticTo` passes all four coordinate values directly to JUCE's `Path::quadraticTo` without the `SANITIZED()` macro applied. Other path methods like `startNewSubPath`, `lineTo`, `addArc`, and `addPieSegment` sanitize their coordinate inputs. Passing NaN/Inf values to `quadraticTo` corrupts the path geometry silently. Same root cause as `cubicTo`.
- **Expected:** Apply `SANITIZED()` to all coordinate values, consistent with `startNewSubPath` and `lineTo`.

### Path.getRatio -- returns Infinity/NaN for zero-height paths without validation

- **Type:** missing-validation
- **Severity:** low
- **Location:** ScriptingGraphics.cpp:~1130
- **Observed:** `getRatio()` computes `bounds.getWidth() / bounds.getHeight()` without checking for zero height. A purely horizontal line or empty path produces Infinity or NaN. No error or warning is reported. The result silently propagates into subsequent calculations (e.g., `scaleToFit` with `preserveProportions`).
- **Expected:** Check for zero height before dividing. Return 1.0 as a safe default or report a script error for degenerate paths.

### Path.loadFromData -- silently ignores unsupported data types

- **Type:** missing-validation
- **Severity:** low
- **Location:** ScriptingGraphics.cpp:~1090
- **Observed:** If `data` is not a String, Array, or Path object (e.g., a number, JSON object, or boolean), the method silently does nothing -- the existing path is unchanged with no error reported. The user has no indication that the load failed.
- **Expected:** Report a script error when the data parameter is not one of the three accepted types, e.g., "loadFromData expects a base64 String, byte Array, or Path object".

### Path.createStrokedPath -- non-array dotData silently ignored

- **Type:** missing-validation
- **Severity:** low
- **Location:** ScriptingApiObjects.cpp:~7035
- **Observed:** If `dotData` is not an array (e.g., a number or string), the method silently produces a solid stroke instead of a dashed one. No error or warning is reported.
- **Expected:** Validate that `dotData` is an array and report a script error for non-array input, or at least document that non-array values default to solid stroke.

### Path.getIntersection -- start point Y silently offset by -0.001

- **Type:** code-smell
- **Severity:** low
- **Location:** ScriptingGraphics.cpp:~1170
- **Observed:** The start point Y coordinate is internally adjusted by -0.001 pixels to work around edge cases where the start point lies exactly on the path boundary. This workaround silently affects precision for all intersection tests, not just boundary cases. The 0.001 offset is hardcoded with no way to disable it.
- **Expected:** Either document this offset as intentional behavior, or use JUCE's tolerance parameter instead of modifying the user's input coordinates.

### Graphics.drawFFTSpectrum -- error message says "not a SVG object" instead of "not a FFT object"

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingGraphics.cpp:~2186
- **Observed:** When `drawFFTSpectrum` receives a non-FFT object, the error message says "not a SVG object" instead of "not a FFT object". This is a copy-paste error from the `drawSVG` method immediately below it in the source file.
- **Expected:** Change the error message to "not a FFT object".

### Graphics.applyVignette -- error message says "applySepia" instead of "applyVignette"

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingGraphics.cpp:~1890
- **Observed:** When `applyVignette` is called without an active layer, the error message says "You need to create a layer for applySepia" instead of "You need to create a layer for applyVignette". This is a copy-paste error from the `applySepia` method just above it.
- **Expected:** Change the error message to "You need to create a layer for applyVignette".

### Rectangle.withAspectRatioLike -- division by zero on zero-width input rectangle

- **Type:** missing-validation
- **Severity:** low
- **Location:** RectangleDynamicObject.cpp:~149
- **Observed:** When `otherRect` has zero width, the aspect ratio computation `other.getHeight() / other.getWidth()` produces infinity or NaN. The resulting rectangle has non-finite coordinates. No error or warning is reported.
- **Expected:** Check for zero width before dividing, and either report a script error or return a copy of the original rectangle unchanged (consistent with the invalid-argument fallback pattern used by other Rectangle methods).

### MessageHolder.addToTimestamp -- method not registered in constructor

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApiObjects.cpp:~5522-5580
- **Observed:** `addToTimestamp` is declared in the header (line 1592), implemented in the .cpp (line 5654), and listed in the Doxygen-generated base JSON, but is NOT registered via `ADD_API_METHOD_1` or `ADD_TYPED_API_METHOD_1` in the constructor, and is NOT in the Wrapper struct. The method is inaccessible from HISEScript despite appearing in the API documentation.
- **Expected:** Add `API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, addToTimestamp);` to the Wrapper struct and `ADD_TYPED_API_METHOD_1(addToTimestamp, VarTypeChecker::Number);` (or `ADD_API_METHOD_1(addToTimestamp);`) to the constructor, consistent with the other timestamp methods.

### UserPresetHandler.setPostSaveCallback -- addAsSource debug label says "postCallback" instead of "postSaveCallback"

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptExpansion.cpp:~223
- **Observed:** `setPostSaveCallback` calls `postSaveCallback.addAsSource(this, "postCallback")` using the string `"postCallback"` as the debug source name. This is a copy-paste from the `setPostCallback` implementation above it. Both the post-load callback and the post-save callback share the same debug label.
- **Expected:** Should use `postSaveCallback.addAsSource(this, "postSaveCallback")` so the debug information system distinguishes the two callbacks in the HISE IDE.

### MidiList.setValue -- Doxygen comment claims value range -127 to 128

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApiObjects.h:~290
- **Observed:** The Doxygen comment on `setValue` says the value parameter should be "between -127 and 128", but the C++ implementation stores values as plain integers with no clamping or validation. Any `int` value works.
- **Expected:** Either add range clamping to match the documentation, or correct the Doxygen comment to reflect that values are unclamped integers.

### Broadcaster.attachToModuleParameter -- error message says "mouse events" instead of "module parameter events"

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptBroadcaster.cpp:~4018
- **Observed:** The error message when the broadcaster has the wrong argument count says `"If you want to attach a broadcaster to mouse events, it needs three parameters (processorId, parameterId, value)"`. The word "mouse events" is incorrect -- this method is for module parameter events. The error was likely copy-pasted from `attachToComponentMouseEvents`.
- **Expected:** Change the error message to `"If you want to attach a broadcaster to module parameter events, it needs three parameters (processorId, parameterId, value)"`.

### Broadcaster.attachToInterfaceSize -- error message says "visibility events" instead of "interface size events"

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptBroadcaster.cpp:~3930
- **Observed:** The error message when the broadcaster has the wrong argument count says `"If you want to attach a broadcaster to visibility events, it needs two parameters (width and height)"`. The word "visibility" is incorrect -- this method is for interface size events, not visibility events. The error was likely copy-pasted from `attachToComponentVisibility`.
- **Expected:** Change the error message to `"If you want to attach a broadcaster to interface size events, it needs two parameters (width and height)"`.

### Broadcaster.attachToSampleMap -- single-module error message says "routing matrix" instead of "samplers"

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptBroadcaster.cpp:~4270
- **Observed:** When a single (non-array) module ID is passed to `attachToSampleMap` and the module is not a `ModulatorSampler`, the error message says `"the modules must have a routing matrix"` instead of `"the modules must be samplers"`. The array branch at line 4260 uses the correct message. The single-module branch at line 4270 was likely copy-pasted from `attachToRoutingMatrix`.
- **Expected:** Change the error message in the single-module branch to `"the modules must be samplers"` to match the array branch.

### Broadcaster.setReplaceThisReference -- flag is stored but never read

- **Type:** code-smell
- **Severity:** low
- **Location:** ScriptBroadcaster.cpp:~4458, ScriptBroadcaster.h:~319
- **Observed:** `setReplaceThisReference` stores the boolean in the `replaceThisReference` member, but no code reads this member. `ScriptTarget::callSync` at line 898 unconditionally uses `var::NativeFunctionArgs(obj, args.getRawDataPointer(), args.size())`, always replacing `this` with the `obj` parameter from `addListener`. Calling `setReplaceThisReference(false)` is a no-op.
- **Expected:** Either read `replaceThisReference` in `ScriptTarget::callSync` (passing `var()` or the broadcaster as the thisObject when `false`), or remove the method and member if the feature is intentionally abandoned.

### MessageHolder.setStartOffset -- no corresponding getStartOffset() method

- **Type:** ux-issue
- **Severity:** low
- **Location:** ScriptingApiObjects.cpp:~5649
- **Observed:** `setStartOffset()` writes the start offset to the HiseEvent, but there is no `getStartOffset()` method on MessageHolder. The value is write-only from the scripting API. Users cannot read back the start offset they set, nor inspect start offsets on events returned by `MidiPlayer.getEventList()` or captured via `Message.store()`.
- **Expected:** Add a `getStartOffset()` method that returns `(int)e.getStartOffset()`, consistent with the getter/setter pattern used by all other HiseEvent fields exposed on MessageHolder.

### Server.callWithGET / Server.callWithPOST -- complex JSON parameters silently mutate global HTTP header

- **Type:** code-smell
- **Severity:** medium
- **Location:** GlobalServer.cpp:~275
- **Observed:** When `getWithParameters()` detects a complex JSON object (containing arrays or nested objects), it sets `extraHeader = "Content-Type: application/json"` on the GlobalServer, overwriting any previously set custom header. This side effect is global and persistent -- it affects all subsequent GET, POST, and download requests until `setHttpHeader()` is called again.
- **Expected:** The Content-Type header for complex JSON should be per-request (set on the PendingCallback's extraHeader) rather than mutating the global state. Alternatively, the global header should be restored after building the URL.

### Broadcaster.setBypassed -- async parameter name is inverted relative to behavior

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptBroadcaster.cpp:~4466
- **Observed:** The third parameter of `setBypassed` is named `async`, but its value is passed directly to `resendLastMessage(var sync)`. The `ApiHelpers::isSynchronous()` boolean fallback interprets `true` as synchronous and `false` as asynchronous. So passing `async = true` actually produces synchronous dispatch, and `async = false` produces asynchronous dispatch -- the opposite of what the parameter name implies.
- **Expected:** Either rename the parameter to `sync` (matching `resendLastMessage`'s semantics), or negate the value before passing it to `resendLastMessage`. Using `SyncNotification`/`AsyncNotification` constants bypasses the boolean ambiguity but the parameter name still misleads users who pass booleans.

### Graphics.beginBlendLayer -- invalid blend mode string silently fails

- **Type:** missing-validation
- **Severity:** low
- **Location:** ScriptingGraphics.cpp:~2110
- **Observed:** When `beginBlendLayer` receives an unrecognized blend mode string, the `gin::BlendMode` lookup returns an invalid index and the method returns without creating a blend layer or reporting any error. Subsequent draw calls go to the parent canvas instead of the intended blend layer. The user has no indication that the layer was not created.
- **Expected:** Report a script error when the blend mode string does not match any of the 25 supported values, e.g., "Invalid blend mode: [name]. Use one of: Normal, Multiply, Screen, ...".

### Graphics.drawImage -- placeholder rendering overwrites current colour state

- **Type:** code-smell
- **Severity:** low
- **Location:** ScriptingGraphics.cpp:~2172
- **Observed:** When `drawImage` cannot find the specified image name, it renders a grey placeholder rectangle with "XXX" text. This placeholder rendering calls `setColour(Colours::grey)` and `setColour(Colours::black)` on the JUCE Graphics context during the render pass, overwriting whatever colour state subsequent draw actions expect. Drawing operations after a failed `drawImage` call use black as the current colour unless `setColour` is called again.
- **Expected:** The placeholder rendering should save and restore the graphics colour state, or use a separate Graphics state scope (e.g., `Graphics::ScopedSaveState`) to avoid side effects on subsequent draw actions.

### Graphics.setGradientFill -- silently ignores arrays with fewer than 6 elements

- **Type:** missing-validation
- **Severity:** low
- **Location:** ScriptingGraphics.cpp:~2202
- **Observed:** When `gradientData` is an Array with fewer than 6 elements, neither the `size() == 6` nor `size() >= 7` branch matches. The method returns without setting a gradient and without reporting an error. The user has no indication that the call had no effect. Non-Array input correctly triggers `reportScriptError("Gradient Data is not sufficient")`.
- **Expected:** Move the "Gradient Data is not sufficient" error to also cover the case where the array has fewer than 6 elements.

### Graphics.setGradientFill -- out-of-bounds array access with odd trailing element count

- **Type:** missing-validation
- **Severity:** low
- **Location:** ScriptingGraphics.cpp:~2229
- **Observed:** The multi-stop gradient loop `for (int i = 7; i < ar.size(); i += 2)` reads `ar[i]` and `ar[i + 1]` without checking that `i + 1 < ar.size()`. When the trailing element count after index 6 is odd (e.g., array sizes 8, 10, 12...), the final iteration reads one element past the array bounds. For example, an array of size 8 reads `ar[8]` which is out of bounds, producing undefined behavior.
- **Expected:** Either validate that `(ar.size() - 7) % 2 == 0` before the loop and report a script error for malformed input, or change the loop condition to `i + 1 < ar.size()`.

### Synth.setClockSpeed -- error message omits 0 as valid value

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApi.cpp:~6522
- **Observed:** The error message says "Unknown clockspeed. Use 1,2,4,8,16 or 32" but 0 (Inactive/disable clock) is also a valid value that is handled by the switch statement. Users who read the error message will not know that 0 can be passed to disable the clock.
- **Expected:** Change the error message to "Unknown clockspeed. Use 0,1,2,4,8,16 or 32" to include the Inactive option.

### Synth.getSlotFX -- ModuleDiagnoser only checks HotswappableProcessor, not DspNetwork::Holder

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApi.cpp:~6170
- **Observed:** The compile-time diagnostic (`ModuleDiagnoser`) only searches `HotswappableProcessor` types, but the runtime method also falls back to searching `DspNetwork::Holder` types. If the user has a `DspNetwork::Holder` with the target name, the runtime search succeeds but the diagnostic incorrectly reports the module as "not found" during compilation, producing a false positive warning in the HISE IDE.
- **Expected:** The `ModuleDiagnoser` should also search `DspNetwork::Holder` types to match the runtime search behavior, or at minimum note that the diagnostic may produce false positives for scriptnode-based slots.

### Synth.setModulatorAttribute -- base JSON description has wrong chainId for PitchModulation

- **Type:** inconsistency
- **Severity:** low
- **Location:** Doxygen-generated base JSON (Synth.json), ScriptingApi.cpp:~6568
- **Observed:** The Doxygen description for `setModulatorAttribute` says "GainModulation = 1, PitchModulation = 0" but the C++ code uses `case ModulatorSynth::PitchModulation` which equals 2. Passing 0 triggers the default case error. The error message in the code correctly says "1= GainModulation, 2=PitchModulation".
- **Expected:** Correct the Doxygen comment to say "GainModulation = 1, PitchModulation = 2".

### Content.addVisualGuide -- 2-element array with both values non-zero adds uninitialized guide

- **Type:** missing-validation
- **Severity:** low
- **Location:** ScriptingApiContent.cpp:~8985-9002
- **Observed:** When a 2-element array is passed with both values non-zero (e.g., `[50, 100]`), neither the horizontal line branch (`x == 0`) nor the vertical line branch (`y == 0`) executes, but `guides.add(std::move(g))` still runs at line 9002. The guide is added with a default-initialized `Type` and empty area, which may cause undefined rendering behavior.
- **Expected:** Either report a script error for invalid 2-element arrays, or skip the `guides.add()` call when neither branch matches.

### Content.callAfterDelay -- wrapper returns isMouseDown() instead of undefined

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApiWrappers.cpp:~1158
- **Observed:** The `callAfterDelay` wrapper function on line 1158 returns `thisObject->isMouseDown()` instead of `var()`. The underlying `callAfterDelay()` method returns `void`, so the wrapper should return `var()` (undefined). This is a copy-paste error from the `isMouseDown` wrapper. Users calling `Content.callAfterDelay()` receive a spurious integer return value (0, 1, or 2) representing the current mouse button state, rather than undefined.
- **Expected:** Change line 1158 from `return thisObject->isMouseDown();` to `return var();` to match the void return type.

### Content.setSuspendTimerCallback -- silently ignores non-function argument

- **Type:** missing-validation
- **Severity:** low
- **Location:** ScriptingApiContent.cpp:~setSuspendTimerCallback
- **Observed:** If the argument is not a valid JavaScript function (checked via `HiseJavascriptEngine::isJavascriptFunction`), the method silently does nothing -- no error is reported and the previous callback (if any) remains active. The user has no indication that the callback registration failed.
- **Expected:** Report a script error when the argument is not a function, e.g., "suspendFunction must be a function".

### ScriptImage.setImageFile -- forceUseRealFile parameter is ignored

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApiContent.cpp:~4132-4152
- **Observed:** The `forceUseRealFile` parameter is declared in the method signature but immediately discarded via `ignoreUnused(forceUseRealFile)`. The image is always loaded through the pool/expansion handler regardless of this parameter's value. Users passing `true` expecting cache bypass get no effect and no warning.
- **Expected:** Either implement the cache-bypass behavior or remove the parameter and deprecate the 2-argument overload in favor of a 1-argument version.

### UnorderedStack.copyTo -- buffer target uses strict less-than size check

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApiObjects.cpp:~copyTo Buffer path
- **Observed:** The Buffer target path checks `data.size() < b->size` (strict less-than). A buffer with exactly the same number of elements as the stack fails silently and returns false. Users would naturally create a buffer matching `size()` and expect it to work.
- **Expected:** Change to `data.size() <= b->size` so an equal-sized buffer is accepted.

### Sampler.isMicPositionPurged -- returns false silently for out-of-range mic indices

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApi.cpp (ScriptingApiSampler::isMicPositionPurged)
- **Observed:** Out-of-range `micIndex` values silently return false instead of reporting an error. The return value is indistinguishable from a legitimately unpurged mic position. The user has no indication that the index was invalid.
- **Expected:** Validate `micIndex` against `0..getNumMicPositions()-1` and report a script error for out-of-range values.

### Sampler.purgeSampleSelection -- wrong error message

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApi.cpp:~4505
- **Observed:** The error message says "purgeMicPosition()" instead of "purgeSampleSelection()" when the sampler handle is invalid.
- **Expected:** Change the error message to "purgeSampleSelection()".

### Sampler.purgeSampleSelection -- null pointer dereference before null check

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApi.cpp:~4500-4509
- **Observed:** The sampler pointer `s` is dereferenced at line 4502 (`s->getNumSounds()` in `ensureStorageAllocated`) before the null check at line 4505 (`if (s == nullptr)`). If the Sampler handle is invalid, this causes undefined behavior (crash). Additionally, the error message says "purgeMicPosition()" instead of "purgeSampleSelection()".
- **Expected:** Move the null check before the first use of `s`. Fix the error message to say "purgeSampleSelection()".

### Sample.duplicateSample -- missing objectExists() check

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~2670
- **Observed:** `duplicateSample()` does not call `objectExists()` before accessing `sound->getData()`. If the underlying sound was removed, `sound` is null and the method dereferences it, causing undefined behavior. Other Sample methods (get, set, deleteSample, replaceAudioFile) check `objectExists()` first.
- **Expected:** Add `if (!objectExists()) { reportScriptError("Sound does not exist"); RETURN_IF_NO_THROW(nullptr); }` at the top of the method.

### Sample.loadIntoBufferArray -- missing objectExists() check

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~2623
- **Observed:** `loadIntoBufferArray()` does not call `objectExists()` before accessing `sound->getNumMultiMicSamples()`. If the underlying sound was removed, `sound` is null and the method dereferences it. Other Sample methods check `objectExists()` first.
- **Expected:** Add `if (!objectExists()) { reportScriptError("Sound does not exist"); RETURN_IF_NO_THROW(var()); }` at the top of the method.

### Sample.replaceAudioFile -- execution continues after reportScriptError for channel validation

- **Type:** silent-fail
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~2780, 2784, 2796
- **Observed:** After `reportScriptError("channel length mismatch: ...")` at line 2780, `reportScriptError("Invalid channel data at index ...")` at line 2784, and `reportScriptError("Invalid channel data at index ...")` at line 2796, execution continues without a return statement. In non-throwing builds, this proceeds with null channel pointers or inconsistent buffer lengths, leading to undefined behavior in `setDataToReferTo`.
- **Expected:** Add `RETURN_IF_NO_THROW(false);` after each `reportScriptError` call in the channel validation loop.

### Sample.setFromJSON -- silently ignores non-object input

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~2560-2576
- **Observed:** When the `object` parameter is not a DynamicObject (e.g., an array, string, number), the `if (auto dyn = object.getDynamicObject())` check fails and the method silently returns without modifying any properties or reporting an error.
- **Expected:** Report a script error when the input is not a JSON object, e.g., "setFromJSON expects a JSON object".

### Sample.refersToSameSample -- typo in error message

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApiObjects.cpp:~2823
- **Observed:** The error message reads "refersToSampleSample: otherSample parameter is not a sample object" -- "SampleSample" instead of "SameSample".
- **Expected:** Change to "refersToSameSample: otherSample parameter is not a sample object".

### Sample.getId -- no bounds checking on property index

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~2723-2726
- **Observed:** `getId(int id)` directly indexes into `sampleIds[id]` without checking that `id` is within `0..sampleIds.size()-1`. An out-of-range index accesses out-of-bounds memory in the juce::Array, which may crash or return garbage.
- **Expected:** Validate that `id >= 0 && id < sampleIds.size()` and report a script error for out-of-range values.

### SlotFX.swap -- silently returns false for HardcodedSwappableEffect targets

- **Type:** missing-validation
- **Severity:** medium
- **Location:** ScriptingApiObjects.cpp:~3760-3780
- **Observed:** The `swap()` method checks `dynamic_cast<SlotFX*>` on the other slot's underlying processor. If the other slot wraps a `HardcodedSwappableEffect` (which also implements `HotswappableProcessor` and is a valid SlotFX script handle), the cast fails and swap silently returns false without an error message. The user has no indication that the swap did not occur.
- **Expected:** Either support swapping between SlotFX and HardcodedSwappableEffect types, or report a script error when the target is not a plain SlotFX module, e.g., "swap only works between SlotFX module instances".

### SlotFX.setBypassed -- declared but never registered or implemented

- **Type:** code-smell
- **Severity:** low
- **Location:** ScriptingApiObjects.h:~2120
- **Observed:** `setBypassed(bool)` is declared in the ScriptingSlotFX class header with a Doxygen comment, but has no implementation in any .cpp file, no Wrapper struct entry, and no ADD_API_METHOD registration. The Doxygen parser picks it up and includes it in auto-generated API docs, but it cannot be called from HiseScript. Users attempting `slot.setBypassed(true)` get a confusing "method not found" error.
- **Expected:** Either remove the dead declaration from the header, or implement and register it (delegating to the wrapped effect's `setSoftBypass`).

### Builder.connectToScript -- silent failure for non-script modules

- **Type:** silent-fail
- **Severity:** low
- **Location:** ScriptingApiObjects.cpp:~10390-10399
- **Observed:** If the module at `buildIndex` is not a JavascriptProcessor, `connectToScript` silently does nothing. The C++ returns `false` to indicate failure, but the `API_VOID_METHOD_WRAPPER_2` discards the return value, so the user has no way to detect that the connection was not made.
- **Expected:** Should report a script error when the target module is not a JavascriptProcessor (e.g., "Module at index N is not a script processor"), or at minimum use `API_METHOD_WRAPPER_2` to return the bool to the script.

### Math.wrap -- returns negative values for deeply negative inputs despite "always positive" docstring

- **Type:** inconsistency
- **Severity:** low
- **Location:** JavascriptEngineMathObject.cpp:~300
- **Observed:** The C++ docstring says "always positive" but the implementation `fmod(value + limit, limit)` only adds `limit` once. For values more negative than `-limit`, the result is still negative. E.g., `wrap(-5.0, 3.0)` computes `fmod(-2.0, 3.0)` which returns -2.0 on most platforms.
- **Expected:** Either use a negative-safe wrap formula (e.g., `fmod(fmod(value, limit) + limit, limit)`) or update the docstring to document the single-offset limitation.

### Parameter.setRangeProperty -- MidPoint constant accepted but silently ignored

- **Type:** inconsistency
- **Severity:** low
- **Location:** NodeBase.cpp:~1102-1110 (setRangeProperty), NodeBase.cpp:~775-790 (constructor constants), ParameterData.cpp:~74-78 (isRangeId)
- **Observed:** The Parameter constructor registers `MidPoint` as a constant via `ADD_PROPERTY_ID_CONSTANT(PropertyIds::MidPoint)`, but `setRangeProperty()` validates the ID against `RangeHelpers::isRangeId()` which only accepts `MinValue`, `MaxValue`, `StepSize`, and `SkewFactor`. Passing `p.MidPoint` to `setRangeProperty()` silently does nothing. Conversely, `SkewFactor` is a valid range property ID but has no constant on Parameter, requiring users to pass the string literal `"SkewFactor"`.
- **Expected:** Either replace the `MidPoint` constant with `SkewFactor` in the constructor, or extend `isRangeId()` to also accept `MidPoint`.

### WavetableController.saveAsAudioFile -- missing error report for null synth

- **Type:** inconsistency
- **Severity:** low
- **Location:** ScriptingApiObjects.cpp:~5278-5305
- **Observed:** When the wavetable synth reference is invalid, the method silently returns. All other methods in this class call `reportScriptError("No wavetable synth")` on failure.
- **Expected:** Add `else { reportScriptError("No wavetable synth"); }` for consistency with the other 7 methods.

### WavetableController.saveAsHwt -- silent failure without error feedback

- **Type:** ux-issue
- **Severity:** low
- **Location:** ScriptingApiObjects.cpp:~5255-5276
- **Observed:** Two failure paths produce no error message: (1) when no wavetable data is loaded (`!v.isValid()`), and (2) when the parameter is not a ScriptFile object (`dynamic_cast` fails). The user has no way to diagnose why the file was not written.
- **Expected:** Call `reportScriptError` on both failure paths with descriptive messages.
