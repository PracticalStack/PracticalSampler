Synth (namespace)

Global namespace for MIDI generation, voice control, module tree access, and timer
callbacks on the parent synth. Automatically available in every script processor --
not user-created. All module tree operations are scoped to the parent synth's subtree
(with a few global-search exceptions for getMidiPlayer, getRoutingMatrix, etc.).

Complexity tiers:
  1. Module references only: getEffect, getSampler, getModulator, getChildSynth,
     getMidiProcessor cached in onInit, then setAttribute / setBypassed in callbacks.
     Covers basic instrument and FX plugin scripts.
  2. MIDI generation: + playNote, noteOffByEventId, noteOffDelayedByEventId,
     addVolumeFade for scripted note triggering. Requires understanding event IDs
     and artificial notes.
  3. Monophonic / glide: + addNoteOn, addPitchFade, isLegatoInterval,
     setFixNoteOnAfterNoteOff, attachNote for legato instruments. Requires careful
     event ID tracking and the two-phase pitch fade pattern.
  4. Builder API and routing: + createBuilder for programmatic module tree
     construction, getRoutingMatrix for multi-output routing.

Practical defaults:
  - Use deferCallbacks(true) when the script is purely a UI controller for a sampler
    or effect chain -- moves MIDI callbacks off the audio thread for safe UI work.
  - Use deferCallbacks(false) (default) when the script modifies MIDI events in real
    time -- legato scripts, arpeggiators, or anything calling Message.ignoreEvent.
  - Store event IDs from playNote/addNoteOn in reg variables. Use noteOffByEventId
    (not the deprecated noteOff) to stop them.
  - For one-shot note previews from UI buttons, use playNote +
    noteOffDelayedByEventId with a fixed sample count (e.g., 44100 for ~1 second).
  - For custom on-screen keyboards, use playNoteFromUI / noteOffFromUI instead of
    playNote / noteOffByEventId -- routes through the real MIDI pipeline and updates
    keyboard state correctly.
  - startTimer(0.05) (50ms) is a good default for UI-update timers.

Common mistakes:
  - Calling get*() methods (getEffect, getModulator, etc.) outside onInit -- most
    allocate wrapper objects and are restricted to onInit. Store references as
    top-level const var variables.
  - Using playNote for custom keyboard panels -- use playNoteFromUI / noteOffFromUI
    instead. playNote creates artificial events that bypass keyboard state tracking.
  - Using deprecated noteOff(noteNumber) -- unreliable with overlapping voices. Use
    noteOffByEventId with stored event IDs.
  - Calling addPitchFade once for glide -- use the two-phase pattern:
    addPitchFade(id, 0, -delta, 0) then addPitchFade(id, glideTime, 0, 0).
  - Forgetting setShouldKillRetriggeredNote(false) in unison scripts -- without it,
    the synth kills existing voices when a new note arrives on the same pitch.
  - Using chainId 0 for addModulator / setModulatorAttribute -- valid values are
    1 (GainModulation) and 2 (PitchModulation).
  - Using 0-based macroIndex for setMacroControl -- it is 1-based (1-8).
  - playNote rejects velocity 0 with a script error. Use noteOffByEventId to stop
    notes, not zero-velocity note-on.

Example:
  // Synth is a global namespace -- no creation needed.
  // Store module references in onInit:
  const var lfo = Synth.getModulator("LFO1");
  const var fx = Synth.getEffect("Delay1");

Methods (59):
  addController              addEffect
  addMessageFromHolder       addModulator
  addNoteOff                 addNoteOn
  addPitchFade               addToFront
  addVolumeFade              attachNote
  createBuilder              deferCallbacks
  getAllEffects               getAllModulators
  getAttribute               getAudioSampleProcessor
  getChildSynth              getChildSynthByIndex
  getDisplayBufferSource     getEffect
  getIdList                  getMidiPlayer
  getMidiProcessor           getModulator
  getModulatorIndex          getNumChildSynths
  getNumPressedKeys          getRoutingMatrix
  getSampler                 getSliderPackProcessor
  getSlotFX                  getTableProcessor
  getTimerInterval           getWavetableController
  isArtificialEventActive    isKeyDown
  isLegatoInterval           isSustainPedalDown
  isTimerRunning             noteOffByEventId
  noteOffDelayedByEventId    noteOffFromUI
  playNote                   playNoteFromUI
  playNoteWithStartOffset    removeEffect
  removeModulator            sendController
  sendControllerToChildSynths setAttribute
  setClockSpeed              setFixNoteOnAfterNoteOff
  setMacroControl            setModulatorAttribute
  setShouldKillRetriggeredNote setVoiceGainValue
  setVoicePitchValue         startTimer
  stopTimer
