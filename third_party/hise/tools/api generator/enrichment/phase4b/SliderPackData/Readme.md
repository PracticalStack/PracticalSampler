SliderPackData (object)
Obtain via: Engine.createAndRegisterSliderPackData(index)

Scriptable handle to a discrete float value array for step sequencers,
arpeggiators, and multi-slider data. Acts as data model in a model-view
separation -- exists independently of any ScriptSliderPack UI component.
Supports [] operator syntax, undo/redo, Base64 serialization, and data linking.

Complexity tiers:
  1. Basic: setNumSliders, setValue, getValue, setRange. Single slider pack
     for parameter editing or simple data display.
  2. Undo-aware editing: + setValueWithUndo, setAllValuesWithUndo. Interactive
     step editing with full undo/redo for recording, randomization, patterns.
  3. Bulk data management: + setUsePreallocatedLength, getDataAsBuffer,
     toBase64/fromBase64, setAllValues with arrays. Batch operations,
     custom preset serialization, efficient buffer ops for MIDI generation.

Practical defaults:
  - Use setUsePreallocatedLength() immediately after creation when the slider
    count will change at runtime. Avoids reallocation and preserves values.
  - Initialize step sequencer values to 0.0 explicitly -- internal default
    is 1.0, not 0.0. Call setAllValues(0.0) after setNumSliders().
  - Use setValueWithUndo() for user-initiated edits (click, record, randomize).
    Reserve setValue() for non-interactive programmatic updates.
  - Use setAllValuesWithUndo() for bulk operations the user should be able
    to reverse (clear, paste, randomize).

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
  - Assuming new sliders initialize to 0.0 -- they initialize to 1.0.
    Call setAllValues(0.0) explicitly if zero-initialization is needed.
  - Using setValue() for user-initiated edits -- cannot be undone. Use
    setValueWithUndo() so Ctrl+Z works.
  - Creating variable-length sequencer packs without preallocation -- every
    setNumSliders() call allocates a new buffer. Use
    setUsePreallocatedLength(maxSteps) after creation.
  - Passing a Table or AudioFile to linkTo() -- causes type mismatch error.
    Must pass another SliderPackData.
  - Out-of-range getValue() silently returns 1.0 (default) instead of
    throwing -- masks off-by-one bugs.

Example:
  // Create a SliderPackData with 8 steps in range [0, 1]
  const var spd = Engine.createAndRegisterSliderPackData(0);
  spd.setNumSliders(8);
  spd.setRange(0.0, 1.0, 0.1);
  spd.setAllValues(0.5);

  // Read and write individual values
  spd.setValue(0, 1.0);
  var firstVal = spd.getValue(0);

  // Use [] operator syntax
  spd[3] = 0.75;
  var fourthVal = spd[3];

  // Listen for changes
  spd.setContentCallback(function(sliderIndex)
  {
      Console.print("Slider " + sliderIndex + " changed");
  });

Methods (17):
  fromBase64                    getDataAsBuffer
  getCurrentlyDisplayedIndex    getNumSliders
  getValue                      linkTo
  setAllValues                  setAllValuesWithUndo
  setAssignIsUndoable           setContentCallback
  setDisplayCallback            setNumSliders
  setRange                      setUsePreallocatedLength
  setValue                      setValueWithUndo
  toBase64
