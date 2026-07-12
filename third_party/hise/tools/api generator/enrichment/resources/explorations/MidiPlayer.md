# MidiPlayer Infrastructure Context Exploration

## Resource Files Consulted

| File | Lines Read |
|------|-----------|
| `hi_scripting/scripting/api/ScriptingApiObjects.h` | 3023-3278 (ScriptedMidiPlayer class) |
| `hi_scripting/scripting/api/ScriptingApiObjects.cpp` | 6201-6884 (Wrapper, constructor, implementations), 9744-9782 (PlaybackUpdater) |
| `hi_core/hi_dsp/modules/MidiPlayer.h` | 1-786 (entire file) |
| `hi_core/hi_dsp/modules/MidiPlayer.cpp` | 1-2640 (entire file) |
| `hi_scripting/scripting/api/ScriptingApi.cpp` | 6263-6276 (getMidiPlayer factory) |
| `hi_scripting/scripting/api/ScriptingApi.h` | 1106-1112 (typedef), 1296 (getMidiPlayer declaration) |
| `hi_core/hi_core/MainController.h` | 2011-2014 (getGlobalPlaybackSpeed) |
| `enrichment/resources/survey/class_survey.md` | Prerequisites table |

---

## 1. Class Declaration - Full Inheritance Chain and Inner Types

### ScriptedMidiPlayer class declaration

```cpp
class ScriptedMidiPlayer : public MidiPlayerBaseType,
                           public ConstScriptingObject,
                           public SuspendableTimer
{
public:
    ScriptedMidiPlayer(ProcessorWithScriptingContent* p, MidiPlayer* player_);
    ~ScriptedMidiPlayer();

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("MidiPlayer"); };
    Identifier getObjectName() const override { return getClassName(); }

    String getDebugValue() const override;

    void sequenceLoaded(HiseMidiSequence::Ptr newSequence) override;
    void sequencesCleared() override;
    void timerCallback() override;

    // ... API methods ...

    struct Wrapper;

private:
    struct ScriptEventRecordProcessor;
    struct PlaybackUpdater;
    
    ScopedPointer<ScriptEventRecordProcessor> recordEventProcessor;
    void callUpdateCallback();
    ScopedPointer<PlaybackUpdater> playbackUpdater;
    WeakCallbackHolder updateCallback;
    bool useTicks = false;
    bool repaintOnPlaybackChange = false;
    double lastPlaybackChange = 0.0;
    WeakReference<ConstScriptingObject> connectedPanel;

    bool sequenceValid() const { return getPlayer() != nullptr && getSequence() != nullptr; }
    HiseMidiSequence::Ptr getSequence() const { return getPlayer()->getCurrentSequence(); }
};
```

### Inheritance chain:
1. **MidiPlayerBaseType** - Provides `getPlayer()`, registered as `MidiPlayer::SequenceListener`, calls `addSequenceListener(this)` in constructor
2. **ConstScriptingObject** - HISE scripting object base, provides `addConstant()`, `ADD_API_METHOD_N()`, `reportScriptError()`, `getProcessor()`
3. **SuspendableTimer** (JUCE timer variant) - Used for periodic `timerCallback()` to repaint panel on playback position change

### Typedef in ScriptingApi.h
```cpp
typedef ScriptingObjects::ScriptedMidiPlayer ScriptMidiPlayer;
```

### Factory method (ScriptingApi.cpp line 6263)
```cpp
ScriptingObjects::ScriptedMidiPlayer* ScriptingApi::Synth::getMidiPlayer(const String& playerId)
{
    auto p = ProcessorHelpers::getFirstProcessorWithName(
        getScriptProcessor()->getMainController_()->getMainSynthChain(), playerId);

    if (p == nullptr)
        reportScriptError(playerId + " was not found");

    if (auto mp = dynamic_cast<MidiPlayer*>(p))
        return new ScriptingObjects::ScriptedMidiPlayer(getScriptProcessor(), mp);
    else
        reportScriptError(playerId + " is not a MIDI Player");

    RETURN_IF_NO_THROW(new ScriptingObjects::ScriptedMidiPlayer(getScriptProcessor(), nullptr));
}
```

**No onInit-only restriction** - there is no `checkIfSynchronous("getMidiPlayer")` guard.

---

## 2. Constructor Analysis - ALL addConstant() and API Method Registrations

### Constructor (ScriptingApiObjects.cpp line 6250)

```cpp
ScriptingObjects::ScriptedMidiPlayer::ScriptedMidiPlayer(ProcessorWithScriptingContent* p, MidiPlayer* player_):
    MidiPlayerBaseType(player_),
    ConstScriptingObject(p, 0),    // numConstants = 0
    updateCallback(p, this, var(), 1)
{
```

**ZERO `addConstant()` calls.** No PlayState values or other constants are exposed on this object.

### API Method Registrations (ALL of them)

Every method uses `ADD_API_METHOD_N` (untyped) except three:

```cpp
    ADD_API_METHOD_0(getPlaybackPosition);
    ADD_API_METHOD_1(setPlaybackPosition);
    ADD_API_METHOD_1(getNoteRectangleList);
    ADD_API_METHOD_1(connectToPanel);
    ADD_API_METHOD_1(setRepaintOnPositionChange);
    ADD_API_METHOD_0(getEventList);
    ADD_API_METHOD_1(getEventListFromSequence);
    ADD_API_METHOD_1(flushMessageList);
    ADD_API_METHOD_2(flushMessageListToSequence);
    ADD_API_METHOD_0(reset);
    ADD_API_METHOD_0(undo);
    ADD_API_METHOD_0(redo);
    ADD_API_METHOD_1(play);
    ADD_API_METHOD_2(convertEventListToNoteRectangles);
    ADD_API_METHOD_1(stop);
    ADD_API_METHOD_1(record);
    ADD_API_METHOD_3(setFile);
    ADD_API_METHOD_2(saveAsMidiFile);
    ADD_API_METHOD_0(getMidiFileList);
    ADD_API_METHOD_1(setTrack);
    ADD_API_METHOD_1(setSequence);
    ADD_API_METHOD_0(isEmpty);
    ADD_API_METHOD_3(create);
    ADD_API_METHOD_0(getNumTracks);
    ADD_API_METHOD_0(getNumSequences);
    ADD_API_METHOD_0(getPlayState);
    ADD_API_METHOD_0(getTimeSignature);
    ADD_API_METHOD_1(setTimeSignature);
    ADD_API_METHOD_1(getTimeSignatureFromSequence);
    ADD_API_METHOD_2(setTimeSignatureToSequence);
    ADD_API_METHOD_1(setSyncToMasterClock);
    ADD_API_METHOD_1(setUseTimestampInTicks);
    ADD_API_METHOD_0(getTicksPerQuarter);
    ADD_API_METHOD_0(getLastPlayedNotePosition);
    ADD_API_METHOD_1(setAutomationHandlerConsumesControllerEvents);
    
    // TYPED API METHODS:
    ADD_TYPED_API_METHOD_1(setSequenceCallback, VarTypeChecker::Function);
    ADD_CALLBACK_DIAGNOSTIC(updateCallback, setSequenceCallback, 0);
    
    ADD_API_METHOD_0(asMidiProcessor);
    ADD_API_METHOD_1(setGlobalPlaybackRatio);
    
    ADD_TYPED_API_METHOD_2(setPlaybackCallback, VarTypeChecker::Function, VarTypeChecker::Number);
    ADD_CALLBACK_DIAGNOSTIC_RAW(setPlaybackCallback, WeakCallbackHolder::checkCallbackNumArgs<2>);
    
    ADD_TYPED_API_METHOD_1(setRecordEventCallback, VarTypeChecker::Function);
    ADD_CALLBACK_DIAGNOSTIC_RAW(setRecordEventCallback, WeakCallbackHolder::checkCallbackNumArgs<1>);
    
    ADD_API_METHOD_1(setUseGlobalUndoManager);
    ADD_API_METHOD_1(connectToMetronome);
    ADD_API_METHOD_1(isSequenceEmpty);
    ADD_API_METHOD_0(clearAllSequences);
```

### Forced Type Map Summary

| Method | Param 1 | Param 2 |
|--------|---------|---------|
| `setSequenceCallback` | Function | -- |
| `setPlaybackCallback` | Function | Number |
| `setRecordEventCallback` | Function | -- |

### Callback Diagnostics

| Registration | Details |
|-------------|---------|
| `setSequenceCallback` | `ADD_CALLBACK_DIAGNOSTIC(updateCallback, setSequenceCallback, 0)` |
| `setPlaybackCallback` | `ADD_CALLBACK_DIAGNOSTIC_RAW(..., WeakCallbackHolder::checkCallbackNumArgs<2>)` - Must have 2 params |
| `setRecordEventCallback` | `ADD_CALLBACK_DIAGNOSTIC_RAW(..., WeakCallbackHolder::checkCallbackNumArgs<1>)` - Must have 1 param |

### Wrapper struct return types (API_METHOD vs API_VOID_METHOD)

Methods that return values (API_METHOD_WRAPPER_N):
- `getPlaybackPosition`, `getNoteRectangleList`, `getEventList`, `getEventListFromSequence`,
  `convertEventListToNoteRectangles`, `saveAsMidiFile`, `play`, `stop`, `record`, `setFile`,
  `getMidiFileList`, `isEmpty`, `getNumTracks`, `getNumSequences`, `getTicksPerQuarter`,
  `getPlayState`, `getTimeSignature`, `setTimeSignature`, `getTimeSignatureFromSequence`,
  `setTimeSignatureToSequence`, `getLastPlayedNotePosition`, `asMidiProcessor`, `isSequenceEmpty`

Methods that return void (API_VOID_METHOD_WRAPPER_N):
- `setPlaybackPosition`, `connectToPanel`, `setRepaintOnPositionChange`, `flushMessageList`,
  `flushMessageListToSequence`, `reset`, `undo`, `redo`, `setTrack`, `setSequence`, `create`,
  `setUseTimestampInTicks`, `setSyncToMasterClock`, `setSequenceCallback`,
  `setAutomationHandlerConsumesControllerEvents`, `setGlobalPlaybackRatio`, `setPlaybackCallback`,
  `setRecordEventCallback`, `setUseGlobalUndoManager`, `connectToMetronome`, `clearAllSequences`

---

## 3. HiseMidiSequence - The MIDI Data Container

### Class declaration (MidiPlayer.h)

```cpp
class HiseMidiSequence : public ReferenceCountedObject,
                         public RestorableObject
{
public:
    enum class TimestampEditFormat
    {
        Samples,
        Ticks,
        numTimestampFormats
    };

    struct TimeSignature : public RestorableObject
    {
        double numBars = 0.0;
        double nominator = 4.0;
        double denominator = 4.0;
        double bpm = 120.0;
        Range<double> normalisedLoopRange = { 0.0, 1.0 };

        void setLoopEnd(double normalisedEnd);
        void setLoopStart(double normalisedStart);
        void calculateNumBars(double lengthInQuarters, bool roundToQuarter);
        String toString() const;
        double getNumQuarters() const;
        ValueTree exportAsValueTree() const override;
        var getAsJSON() const;
        void restoreFromValueTree(const ValueTree &v) override;
    };

    static constexpr int TicksPerQuarter = 960;

    using Ptr = ReferenceCountedObjectPtr<HiseMidiSequence>;
    using List = ReferenceCountedArray<HiseMidiSequence>;

private:
    TimestampEditFormat timestampFormat = TimestampEditFormat::Samples;
    TimeSignature signature;
    mutable SimpleReadWriteLock swapLock;
    Identifier id;
    OwnedArray<MidiMessageSequence> sequences;  // TRACKS (one per track)
    int currentTrackIndex = 0;
    int lastPlayedIndex = -1;
    double artificialLengthInQuarters = -1.0;
};
```

### Key characteristics:
- **TicksPerQuarter = 960** (constant, exposed by `getTicksPerQuarter()`)
- Reference counted (`Ptr = ReferenceCountedObjectPtr<HiseMidiSequence>`)
- **Tracks** stored as `OwnedArray<MidiMessageSequence> sequences` - multiple tracks per sequence
- Thread safety: `SimpleReadWriteLock swapLock` protects sequence data
- `artificialLengthInQuarters`: When -1.0, length computed from `signature` or event data

### Length calculation (MidiPlayer.cpp line 332):
```cpp
double HiseMidiSequence::getLength() const
{
    SimpleReadWriteLock::ScopedReadLock sl(swapLock);
    if (artificialLengthInQuarters != -1.0)
        return artificialLengthInQuarters * (double)TicksPerQuarter;
    if (signature.numBars != 0.0)
        return signature.getNumQuarters() * (double)TicksPerQuarter;
    double maxLength = 0.0;
    for (auto seq : sequences)
        maxLength = jmax(maxLength, seq->getEndTime());
    return maxLength;
}
```

### TimeSignature::getNumQuarters:
```cpp
double HiseMidiSequence::TimeSignature::getNumQuarters() const
{
    return numBars / denominator * 4.0 * nominator;
}
```

### getEventList - converting internal MIDI to HiseEvents:
```cpp
Array<HiseEvent> HiseMidiSequence::getEventList(double sampleRate, double bpm,
    HiseMidiSequence::TimestampEditFormat formatToUse)
```
- Iterates current track's `MidiMessageSequence`
- For note on/off pairs: converts timestamps from ticks to either samples or ticks
- Also handles CC and pitch wheel events
- Sorts output by timestamp
- If `formatToUse == numTimestampFormats`, falls back to `timestampFormat`

### getRectangleList - for UI visualization:
```cpp
RectangleList<float> HiseMidiSequence::getRectangleList(Rectangle<float> targetBounds) const
```
- Creates rectangles: x = noteOnTime/length, w = (noteOffTime-noteOnTime)/length, y = (127-noteNumber)/128, h = 1/128
- Scales to `targetBounds` using AffineTransform

---

## 4. MidiPlayer Core Class

### Class declaration (MidiPlayer.h line 248)

```cpp
class MidiPlayer : public MidiProcessor,
                   public TempoListener
```

### PlayState enum:
```cpp
enum class PlayState
{
    Stop,    // = 0
    Play,    // = 1
    Record,  // = 2
    numPlayStates
};
```

### RecordState enum (internal):
```cpp
enum class RecordState
{
    Idle,
    PreparationPending,
    Prepared,
    FlushPending,
    numRecordStates
};
```

### SpecialParameters enum:
```cpp
enum SpecialParameters
{
    CurrentPosition,       ///< the current position within the current MIDI file (non-persistent)
    CurrentSequence,       ///< the index of the currently played sequence (not zero based for combobox compatibility)
    CurrentTrack,          ///< the index of the currently played track within a sequence.
    LoopEnabled,           ///< toggles between oneshot and loop playback
    LoopStart,             ///< start of the (loop) playback
    LoopEnd,               ///< end of the (loop) playback
    PlaybackSpeed,         ///< the playback speed of the MidiPlayer
    numSpecialParameters
};
```

### Key private members:
```cpp
    bool syncToMasterClock = false;
    mutable SimpleReadWriteLock sequenceLock;
    HiseMidiSequence::List currentSequences;   // Array of loaded sequences
    PlayState playState = PlayState::Stop;
    double ticksSincePlaybackStart = 0.0;
    double currentPosition = -1.0;             // -1 means stopped
    int currentSequenceIndex = -1;
    int currentTrackIndex = 0;
    bool loopEnabled = true;
    int timeStampForNextCommand = 0;
    double ticksPerSample = 0.0;
    double playbackSpeed = 1.0;
    double recordStart = 0.0;
    bool flushRecordedEvents = true;
    ScopedPointer<UndoManager> ownedUndoManager;
    UndoManager* undoManager = nullptr;        // Starts as nullptr! Undo disabled by default
    Array<PoolReference> currentlyLoadedFiles;
    Array<WeakReference<SequenceListener>> sequenceListeners;
    Array<WeakReference<PlaybackListener>> playbackListeners;
    Array<HiseEvent> currentlyRecordedEvents;
    std::atomic<RecordState> recordState{ RecordState::Idle };
    bool overdubMode = true;
    hise::UnorderedStack<NotePair> overdubNoteOns;
```

### Multi-sequence support:
- `currentSequences` is `ReferenceCountedArray<HiseMidiSequence>` (`HiseMidiSequence::List`)
- `currentSequenceIndex` selects the active one (zero-based internally, one-based in scripting API)
- `getAttribute(CurrentSequence)` returns `currentSequenceIndex + 1`
- `setInternalAttribute(CurrentSequence, newAmount)` stores `(int)(newAmount - 1)`

### Playback speed:
```cpp
double MidiPlayer::getPlaybackSpeed() const
{ return playbackSpeed * getMainController()->getGlobalPlaybackSpeed(); }
```
Global speed is a MainController-level multiplier. `setGlobalPlaybackRatio()` on ScriptedMidiPlayer calls `getScriptProcessor()->getMainController_()->setGlobalMidiPlaybackSpeed(globalRatio)`.

---

## 5. MidiPlayerBaseType

```cpp
class MidiPlayerBaseType : public MidiPlayer::SequenceListener
{
public:
    static Identifier getId();
    static MidiPlayerBaseType* create(MidiPlayer* player);
    virtual ~MidiPlayerBaseType();
    virtual int getPreferredHeight() const;
    void setFont(Font f);
    void initMidiPlayer(MidiPlayer* player);

protected:
    void cancelUpdates();    // Removes this as SequenceListener from player
    MidiPlayerBaseType(MidiPlayer* player_);
    MidiPlayer* getPlayer();
    const MidiPlayer* getPlayer() const;

private:
    Font font;
    int lastTrackIndex = 0;
    int lastSequenceIndex = -1;
    WeakReference<MidiPlayer> player;
};
```

Constructor calls `initMidiPlayer(player)` which registers as SequenceListener.
`cancelUpdates()` removes as SequenceListener.

---

## 6. Listener/Callback Interfaces

### MidiPlayer::SequenceListener
```cpp
struct SequenceListener
{
    virtual ~SequenceListener();
    virtual void sequenceLoaded(HiseMidiSequence::Ptr newSequence) = 0;
    virtual void sequencesCleared() = 0;
};
```
**Threading**: "This will always happen on the message thread" (documented in header).

### MidiPlayer::PlaybackListener
```cpp
struct PlaybackListener
{
    virtual ~PlaybackListener();
    virtual void playbackChanged(int timestamp, PlayState newState) = 0;
};
```
**Threading**: Called from `sendPlaybackChangeMessage()` which is called from `startInternal`/`stopInternal`/`recordInternal` -- can be on the audio thread.

### MidiPlayer::EventRecordProcessor
```cpp
struct EventRecordProcessor
{
    virtual ~EventRecordProcessor();
    virtual void processRecordedEvent(HiseEvent& e) = 0;
};
```
**Threading**: Called from `processHiseEvent()` -- audio thread. The callback runs on the audio thread.

### ScriptedMidiPlayer callback wrappers:

**ScriptEventRecordProcessor** (inner struct, audio thread):
```cpp
struct ScriptEventRecordProcessor : public MidiPlayer::EventRecordProcessor
{
    ScriptEventRecordProcessor(ScriptedMidiPlayer& parent_, const var& function):
        parent(parent_),
        eventCallback(parent.getScriptProcessor(), &parent, function, 1),
        mp(parent.getPlayer())
    {
        eventCallback.incRefCount();
        mp->addEventRecordProcessor(this);
        holder = new ScriptingMessageHolder(parent.getScriptProcessor());
        args = var(holder);
    }

    ~ScriptEventRecordProcessor()
    {
        if (mp != nullptr) mp->removeEventRecordProcessor(this);
        holder = nullptr; args = var();
    }

    void processRecordedEvent(HiseEvent& e) override
    {
        holder->setMessage(e);
        var thisObject(&parent);
        eventCallback.callSync(var::NativeFunctionArgs(thisObject, &args, 1));
        e = holder->getMessageCopy();
    }
    // ...
};
```
- Uses `callSync` -- runs synchronously on the audio thread
- Passes a `ScriptingMessageHolder` wrapping the HiseEvent
- The event is mutated: `e = holder->getMessageCopy()` after callback
- Realtime safety check in `setRecordEventCallback`:
```cpp
#if USE_BACKEND
    if (HiseJavascriptEngine::RootObject::RealtimeSafetyInfo::check(co, this, "MidiPlayer.setRecordEventCallback"))
        reportScriptError("This callable object is not safe for audio-thread execution");
#else
    if (!co->isRealtimeSafe())
        reportScriptError("This callable object is not realtime safe!");
#endif
```

**PlaybackUpdater** (inner struct):
```cpp
struct PlaybackUpdater : public PooledUIUpdater::SimpleTimer,
                         public MidiPlayer::PlaybackListener
{
    PlaybackUpdater(ScriptedMidiPlayer& parent_, var f, bool sync_);
    ~PlaybackUpdater();
    void timerCallback() override;
    void playbackChanged(int timestamp, MidiPlayer::PlayState newState) override;

    bool dirty = false;
    const bool sync;
    ScriptedMidiPlayer& parent;
    WeakCallbackHolder playbackCallback;
    var args[2];
};
```

Implementation (ScriptingApiObjects.cpp line 9744):
```cpp
PlaybackUpdater::PlaybackUpdater(ScriptedMidiPlayer& parent_, var f, bool sync_) :
    SimpleTimer(parent_.getScriptProcessor()->getMainController_()->getGlobalUIUpdater(), !sync_),
    sync(sync_),
    parent(parent_),
    playbackCallback(parent.getScriptProcessor(), &parent, f, 2)
{
    if (auto mp = parent.getPlayer())
        mp->addPlaybackListener(this);
    playbackCallback.incRefCount();
    playbackCallback.setThisObject(&parent);
    playbackCallback.addAsSource(&parent, "onPlaybackChange");
}

void PlaybackUpdater::playbackChanged(int timestamp, MidiPlayer::PlayState newState)
{
    args[0] = var(timestamp);
    args[1] = var((int)newState);
    if (sync)
        playbackCallback.callSync(args, 2, nullptr);
    else
        dirty = true;
}

void PlaybackUpdater::timerCallback()
{
    if (dirty)
    {
        playbackCallback.call(args, 2);
        dirty = false;
    }
}
```

**Key**: When `sync` is true (second arg to `setPlaybackCallback` is non-zero), `callSync` is called on the audio thread. When false, callback is deferred to UI thread via `dirty` flag.

**setSequenceCallback**: Uses `WeakCallbackHolder updateCallback` with 1 argument:
```cpp
void ScriptedMidiPlayer::callUpdateCallback()
{
    if (updateCallback)
    {
        var thisVar(this);
        updateCallback.call(&thisVar, 1);
    }
}
```
Uses `.call()` (async/deferred), not `callSync`. Argument is `this` (the MidiPlayer object).

---

## 7. Transport State Machine

### Public transport methods (MidiPlayer core):

```cpp
bool MidiPlayer::play(int timestamp)
{
    if (syncToMasterClock && !isRecording())
        return false;    // No-op when synced and not recording
    return startInternal(timestamp);
}

bool MidiPlayer::stop(int timestamp)
{
    if (syncToMasterClock)
    {
        recordOnNextPlaybackStart = false;
        return false;    // No-op when synced
    }
    return stopInternal(timestamp);
}

bool MidiPlayer::record(int timestamp)
{
    if (syncToMasterClock && getPlayState() == PlayState::Stop)
    {
        recordOnNextPlaybackStart = true;  // Defers until master clock starts
        return false;
    }
    return recordInternal(timestamp);
}
```

### startInternal:
- If recording in overdub mode, just switches to Play
- Otherwise resets position to 0, sets `playState = PlayState::Play`
- Calls `sendPlaybackChangeMessage(timestamp)` which notifies PlaybackListeners

### stopInternal:
- If recording, calls `finishRecording()`
- If `noteOffAtStop`, adds note-offs for pending notes
- Handles sustain pedal cleanup
- Resets playback, sets `playState = PlayState::Stop`, `currentPosition = -1.0`
- Calls `sendPlaybackChangeMessage(timestamp)`

### recordInternal:
- Starts overdub updater if in overdub mode
- If stopped, resets position to 0
- Sets `playState = PlayState::Record`
- Calls `sendPlaybackChangeMessage(timestamp)`
- If `recordState == Idle`, calls `prepareForRecording(true)`

### Master clock sync behavior:
When `syncToMasterClock == true`:
- `play()`, `stop()` return false (no-op)
- `record()` from stop sets `recordOnNextPlaybackStart = true` then returns false
- Transport driven by `onGridChange()` and `onTransportChange()` from TempoListener
- `onGridChange()` starts or records on first grid event, re-syncs position
- `onTransportChange(false, ...)` stops playback

---

## 8. Threading & Lifecycle

### Audio thread operations:
- `preprocessBuffer()` - Main playback engine. Iterates MIDI events, adds to HiseEventBuffer, advances position
- `processHiseEvent()` - Records incoming MIDI events during recording
- `EventRecordProcessor::processRecordedEvent()` - Audio thread callback for filtering recorded events
- `sendPlaybackChangeMessage()` - Called from start/stop/record which can be audio thread

### Message thread operations:
- `sequenceLoaded()` / `sequencesCleared()` - Always message thread (documented)
- `timerCallback()` on ScriptedMidiPlayer (SuspendableTimer) - Message thread
- `Updater::timerCallback()` - UI thread (deferred sequence update notifications)
- `PlaybackUpdater::timerCallback()` - UI thread (async playback change callbacks)

### SuspendableTimer usage:
- `setRepaintOnPositionChange(true)` calls `startTimer(50)` (50ms)
- `setRepaintOnPositionChange(false)` calls `stopTimer()`
- `timerCallback()` checks if position changed, repaints connected panel

### Data locking:
- `sequenceLock` (SimpleReadWriteLock) - Protects `currentSequences` array
- `swapLock` on HiseMidiSequence - Protects internal track data
- `listenerLock` - Protects `sequenceListeners` array
- `overdubLock` - Protects `overdubNoteOns`

---

## 9. Preprocessor Guards

### USE_BACKEND guards:
In `setRecordEventCallback`:
```cpp
#if USE_BACKEND
    if (HiseJavascriptEngine::RootObject::RealtimeSafetyInfo::check(co, this, "MidiPlayer.setRecordEventCallback"))
        reportScriptError("This callable object is not safe for audio-thread execution");
#else
    if (!co->isRealtimeSafe())
        reportScriptError("This callable object is not realtime safe!");
#endif
```

In `setPlaybackCallback`:
```cpp
#if USE_BACKEND
    if (isSync)
    {
        if (auto co = dynamic_cast<WeakCallbackHolder::CallableObject*>(newPlaybackCallback.getObject()))
        {
            if (HiseJavascriptEngine::RootObject::RealtimeSafetyInfo::check(co, this, "MidiPlayer.setPlaybackCallback"))
                reportScriptError("Callback is not safe for synchronous audio-thread execution");
        }
    }
#endif
```

In MidiPlayer::createEditor:
```cpp
#if USE_BACKEND
    return new MidiPlayerEditor(parentEditor);
#else
    ignoreUnused(parentEditor);
    jassertfalse;
    return nullptr;
#endif
```

No `USE_FRONTEND` or `HI_EXPORT_AS_PROJECT_DLL` guards in MidiPlayer-related code.

---

## 10. Tick/Sample Conversion - MidiPlayerHelpers

```cpp
struct MidiPlayerHelpers
{
    static double samplesToTicks(double samples, double bpm, double sr)
    {
        auto samplesPerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sr, TempoSyncer::Quarter);
        return (double)HiseMidiSequence::TicksPerQuarter * samples / samplesPerQuarter;
    }

    static double ticksToSamples(double ticks, double bpm, double sr)
    {
        auto samplesPerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sr, TempoSyncer::Quarter);
        return samplesPerQuarter * ticks / HiseMidiSequence::TicksPerQuarter;
    }
    // ... also samplesToSeconds, secondsToTicks, secondsToSamples
};
```

### Key relationship:
- **TicksPerQuarter = 960** (constant)
- Internal storage always in ticks
- `tempoChanged()`: `ticksPerSample = MidiPlayerHelpers::samplesToTicks(1, newTempo, getSampleRate())`
- `getTicksPerSample()` = `ticksPerSample * getPlaybackSpeed()`

### Scripting timestamp modes:
- `setUseTimestampInTicks(true)` sets `useTicks = true` on ScriptedMidiPlayer
- Affects `getEventList()`, `getEventListFromSequence()`, `flushMessageList()`, `flushMessageListToSequence()`
- When `useTicks == true`, events use tick timestamps
- When `useTicks == false` (default), events use sample timestamps at current sampleRate/BPM

---

## 11. Undo System

### Three UndoableAction types:

#### EditAction
- Captures old events and new events for the current track
- `perform()`: Writes new events via `writeArrayToSequence()`
- `undo()`: Writes old events back
- Both call `updatePositionInCurrentSequence()` and `sendSequenceUpdateMessage(sendNotificationAsync)`

#### SequenceListAction
- Swaps entire `currentSequences` list
- Used when loaded sequences change
- `perform()`: `swapSequenceListWithIndex(newList, newIndex)`
- `undo()`: `swapSequenceListWithIndex(oldList, oldIndex)`

#### TimesigUndo
- Swaps time signature on the current sequence
- `perform()`: `player->setLength(newSig, false)`
- `undo()`: `player->setLength(oldSig, false)`

### Undo manager state:

**Default state**: `undoManager = nullptr` (undo disabled). `ownedUndoManager` is created in constructor but `undoManager` does not point to it.

Two distinct methods on MidiPlayer core:
```cpp
// MidiPlayer.cpp line 817 - preserves owned manager
void MidiPlayer::setUseExternalUndoManager(UndoManager* externalUndoManagerToUse)
{
    if (externalUndoManagerToUse == nullptr)
        undoManager = ownedUndoManager.get();
    else
        undoManager = externalUndoManagerToUse;
}

// MidiPlayer.cpp line 1875 - DESTROYS owned manager
void MidiPlayer::setExternalUndoManager(UndoManager* externalUndoManager)
{
    ownedUndoManager = nullptr;
    undoManager = externalUndoManager;
}
```

**ScriptedMidiPlayer uses `setExternalUndoManager()`** (the destructive one):
```cpp
void ScriptedMidiPlayer::setUseGlobalUndoManager(bool shouldUseGlobalUndoManager)
{
    if (shouldUseGlobalUndoManager)
        getPlayer()->setExternalUndoManager(getScriptProcessor()->getMainController_()->getControlUndoManager());
    else
        getPlayer()->setExternalUndoManager(nullptr);
}
```

**Consequence**:
- `setUseGlobalUndoManager(true)` -- uses global undo manager, destroys owned one
- `setUseGlobalUndoManager(false)` -- sets both `ownedUndoManager = nullptr` AND `undoManager = nullptr`, **undo disabled permanently**
- `undo()` and `redo()` call `reportScriptError("Undo is deactivated")` when `undoManager == nullptr`

**CRITICAL PITFALL**: By default, undo is disabled (`undoManager = nullptr`). You must call `setUseGlobalUndoManager(true)` before using `undo()`/`redo()`. But once you call `setUseGlobalUndoManager(false)`, undo is permanently disabled for this player because the owned UndoManager is destroyed.

From `flushEdit()`:
```cpp
void MidiPlayer::flushEdit(const Array<HiseEvent>& newEvents, ...)
{
    ScopedPointer<EditAction> newAction = new EditAction(...);
    if (undoManager != nullptr)
    {
        if (ownedUndoManager != nullptr)
            ownedUndoManager->beginNewTransaction();
        undoManager->perform(newAction.release());
    }
    else
        newAction->perform();   // No undo tracking
}
```

---

## 12. JSON Schemas

### TimeSignature JSON format

The `getAsJSON()` method on TimeSignature exports via ValueTree with properties from `TimeSigIds`:
```cpp
namespace TimeSigIds
{
    DECLARE_ID(Nominator);
    DECLARE_ID(Denominator);
    DECLARE_ID(NumBars);
    DECLARE_ID(LoopStart);
    DECLARE_ID(LoopEnd);
    DECLARE_ID(Tempo);
}
```

JSON object format:
```javascript
{
    "Nominator": 4.0,       // numerator of time signature
    "Denominator": 4.0,     // denominator of time signature
    "NumBars": 4.0,         // number of bars
    "LoopStart": 0.0,       // normalised loop start (0.0-1.0)
    "LoopEnd": 1.0,         // normalised loop end (0.0-1.0)
    "Tempo": 120.0          // BPM
}
```

### setTimeSignatureToSequence parsing:
```cpp
bool ScriptedMidiPlayer::setTimeSignatureToSequence(int index, var timeSignatureObject)
{
    if (auto seq = getPlayer()->getSequenceWithIndex(index))
    {
        HiseMidiSequence::TimeSignature sig;
        sig.nominator = timeSignatureObject.getProperty(TimeSigIds::Nominator, 0);
        sig.denominator = timeSignatureObject.getProperty(TimeSigIds::Denominator, 0);
        sig.numBars = timeSignatureObject.getProperty(TimeSigIds::NumBars, 0);
        sig.normalisedLoopRange = {
            (double)timeSignatureObject.getProperty(TimeSigIds::LoopStart, 0.0),
            (double)timeSignatureObject.getProperty(TimeSigIds::LoopEnd, 1.0)
        };
        bool valid = sig.numBars > 0 && sig.nominator > 0 && sig.denominator > 0;
        if(valid) seq->setLengthFromTimeSignature(sig);
        return valid;
    }
    return false;
}
```
**Note**: `Tempo` is NOT parsed back from JSON in `setTimeSignatureToSequence`! Only Nominator, Denominator, NumBars, LoopStart, LoopEnd are consumed.

### Note rectangle format:
Returned by `getNoteRectangleList()` and `convertEventListToNoteRectangles()`:
- Returns `Array<var>` where each element is `[x, y, w, h]` (or `ScriptRectangle` depending on `HISE_USE_SCRIPT_RECTANGLE_OBJECT`)
- Coordinates scaled to supplied `targetBounds`

---

## 13. Enum/Constant Behavioral Tracing

### PlayState values (NOT registered as constants, consumed via getPlayState)

No constants exposed on ScriptedMidiPlayer object (constructor has `numConstants = 0`).

| Value | Enum | Meaning |
|-------|------|---------|
| 0 | `PlayState::Stop` | Player stopped, `currentPosition == -1.0` |
| 1 | `PlayState::Play` | Actively playing MIDI events |
| 2 | `PlayState::Record` | Recording incoming MIDI events |

Passed as second argument to playback callback:
```cpp
void PlaybackUpdater::playbackChanged(int timestamp, MidiPlayer::PlayState newState)
{
    args[0] = var(timestamp);
    args[1] = var((int)newState);  // 0, 1, or 2
}
```

### Where PlayState is consumed:
- `preprocessBuffer()`: `playState == PlayState::Stop` -> reset and return
- `isRecording()`: `return getPlayState() == PlayState::Record`
- `getPlaybackPositionFromTicksSinceStart()`: Returns 0.0 if stopped

### Sync behavior:
When `syncToMasterClock` is true, `play()` and `stop()` are no-ops (return false). Transport controlled by master clock.

### One-based vs zero-based indexing:
- **Sequence indices**: One-based in scripting API. `setSequence(1)` = first sequence. `getSequenceWithIndex(0)` triggers `reportScriptError("Nope. One based!!!")`
- **Track indices**: One-based in scripting API. `setTrack(1)` = first track. `getAttribute(CurrentTrack)` returns `currentTrackIndex + 1`
- `getSequenceWithIndex(-1)` returns `getCurrentSequence()` (special case)

---

## 14. Additional Method Infrastructure

### connectToPanel:
Stores weak reference to ScriptPanel. Panel gets `repaint()` called when:
1. `sequencesCleared()` is triggered
2. Timer callback detects position change (if `repaintOnPlaybackChange == true`)

### connectToMetronome:
Takes string ID, finds `MidiMetronome` processor, calls `typed->connectToPlayer(getPlayer())`.

### asMidiProcessor:
Returns `new ScriptingMidiProcessor(getScriptProcessor(), p)` -- typed MidiProcessor reference for attribute access.

### setGlobalPlaybackRatio:
Calls `getScriptProcessor()->getMainController_()->setGlobalMidiPlaybackSpeed(globalRatio)` -- affects ALL MidiPlayers globally.

### create:
```cpp
void ScriptedMidiPlayer::create(int nominator, int denominator, int barLength)
{
    HiseMidiSequence::TimeSignature sig;
    sig.nominator = nominator;
    sig.denominator = denominator;
    sig.numBars = barLength;
    sig.normalisedLoopRange = { 0.0, 1.0 };
    
    HiseMidiSequence::Ptr seq = new HiseMidiSequence();
    seq->setLengthFromTimeSignature(sig);
    seq->createEmptyTrack();
    getPlayer()->addSequence(seq);  // select=true -> selects the new sequence
}
```
**Appends** a new empty sequence and selects it. Does NOT clear existing sequences.

### setFile:
```cpp
bool ScriptedMidiPlayer::setFile(var fileName, bool clearExistingSequences, bool selectNewSequence)
```
- If `clearExistingSequences`, clears all with `dontSendNotification`
- Loads via pool reference (`FileHandlerBase::MidiFiles`)
- If `selectNewSequence`, sets `CurrentSequence` to the last
- Empty filename with `selectNewSequence` triggers sequence update notification
- Returns true for valid references or empty filenames

### saveAsMidiFile:
- Delegates to `MidiPlayer::saveAsMidiFile(name, trackIndex)` on the core
- Core writes to pool, expands/creates MIDI files with proper track indexing
- Adds time signature and end-of-track meta events
- Reloads pool after writing

### isEmpty:
```cpp
bool ScriptedMidiPlayer::isEmpty() const
{
    return !sequenceValid();
}
```
Returns true if player or current sequence is null.

### isSequenceEmpty:
Delegates to MidiPlayer core. Returns true if sequence doesn't exist or has 0 events.

### setAutomationHandlerConsumesControllerEvents:
Sets `globalMidiHandlerConsumesCC` on core. When true, CC messages from MIDI playback sent to `MidiControlAutomationHandler`.

---

## 15. MidiPlayer::Updater (Sequence Update Notification System)

```cpp
struct Updater : private PooledUIUpdater::SimpleTimer
{
    bool handleUpdate(HiseMidiSequence::Ptr seq, NotificationType n);
    // ...
};
```

- `sendNotificationAsync`: Sets `dirty = true`, timerCallback calls `handleUpdate` with `sendNotificationSync`
- `sendNotificationSync`: Tries listener lock, notifies immediately
- If lock fails on sync path, retries on next timer tick

---

## 16. Recording System Details

### Overdub mode (default: true):
- Note-on events buffered in `overdubNoteOns` (UnorderedStack)
- Note-off events complete pairs
- CC events go to `controllerEvents`
- `OverdubUpdater` (SimpleTimer) periodically flushes completed pairs to sequence

### Event timestamps during recording:
- Overdub mode: timestamps in ticks
- Non-overdub mode: timestamps in samples

### prepareForRecording / finishRecording:
Both run as deferred functions via `getSampleManager().addDeferredFunction()`.

---

## 17. ScriptedMidiPlayer sequenceLoaded / sequencesCleared

```cpp
void ScriptedMidiPlayer::sequenceLoaded(HiseMidiSequence::Ptr newSequence)
{
    callUpdateCallback();
}

void ScriptedMidiPlayer::sequencesCleared()
{
    callUpdateCallback();
    if (connectedPanel != nullptr)
    {
        if (auto sp = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(connectedPanel.get()))
            sp->repaint();
    }
}
```

Both trigger the sequence callback. `sequencesCleared` also repaints the connected panel.
