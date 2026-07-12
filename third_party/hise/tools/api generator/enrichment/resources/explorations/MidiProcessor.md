# MidiProcessor -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- prerequisite table (row 3: Synth -> MidiProcessor)
- `enrichment/resources/survey/class_survey_data.json` -- MidiProcessor entry
- `enrichment/phase1/Synth/Readme.md` -- prerequisite Synth class analysis
- No base class exploration needed (not a component class)

## Class Declaration

**File:** `hi_scripting/scripting/api/ScriptingApiObjects.h:2336-2417`

```cpp
class ScriptingMidiProcessor : public ConstScriptingObject,
                               public AssignableObject
{
public:
    ScriptingMidiProcessor(ProcessorWithScriptingContent *p, MidiProcessor *mp_);
    ~ScriptingMidiProcessor() {};

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("MidiProcessor"); }
    Identifier getObjectName() const override { return getClassName(); }
    bool objectDeleted() const override { return mp.get() == nullptr; }
    bool objectExists() const override { return mp != nullptr; }

    String getDebugName() const override { return mp.get() != nullptr ? mp->getId() : "Invalid"; };
    String getDebugValue() const override { return String(); }

    Component* createPopupComponent(const MouseEvent& e, Component *c) override;

    // AssignableObject interface
    int getCachedIndex(const var &indexExpression) const override;
    void assign(const int index, var newValue) override;
    var getAssignedValue(int /*index*/) const override;

    // API Methods (14 total)
    bool exists();
    String getId() const;
    void setAttribute(int index, float value);
    float getAttribute(int index);
    int getNumAttributes() const;
    String getAttributeId(int index);
    int getAttributeIndex(String id);
    void setBypassed(bool shouldBeBypassed);
    bool isBypassed() const;
    String exportState();
    void restoreState(String base64State);
    String exportScriptControls();
    void restoreScriptControls(String base64Controls);
    var asMidiPlayer();

private:
    WeakReference<MidiProcessor> mp;
};
```

### Inheritance

1. **ConstScriptingObject** -- standard HISE scripting object base providing `checkValidObject()`, `reportScriptError()`, `addConstant()`, `setName()`.
2. **AssignableObject** -- enables bracket-based property access syntax (`mp["paramName"] = value`). Requires implementing `getCachedIndex()`, `assign()`, `getAssignedValue()`.

### Weak Reference Pattern

The class holds a `WeakReference<MidiProcessor> mp` to the underlying C++ processor. Every API method calls `checkValidObject()` before proceeding, returning a default value (0, false, empty string) if the reference is dead.

## Constructor Analysis

**File:** `ScriptingApiObjects.cpp:4561-4594`

```cpp
ScriptingMidiProcessor(ProcessorWithScriptingContent *p, MidiProcessor *mp_) :
    ConstScriptingObject(p, mp_ != nullptr ? mp_->getNumParameters()+1 : 1),
    mp(mp_)
{
    if (mp != nullptr)
    {
        setName(mp->getId());
        addScriptParameters(this, mp.get());

        for (int i = 0; i < mp->getNumParameters(); i++)
        {
            addConstant(mp->getIdentifierForParameterIndex(i).toString(), var(i));
        }
    }
    else
    {
        setName("Invalid MidiProcessor");
    }

    ADD_API_METHOD_2(setAttribute);
    ADD_API_METHOD_1(getAttribute);
    ADD_API_METHOD_1(setBypassed);
    ADD_API_METHOD_0(isBypassed);
    ADD_API_METHOD_0(exportState);
    ADD_API_METHOD_1(restoreState);
    ADD_API_METHOD_0(getId);
    ADD_API_METHOD_1(restoreScriptControls);
    ADD_API_METHOD_0(exportScriptControls);
    ADD_API_METHOD_0(getNumAttributes);
    ADD_API_METHOD_1(getAttributeId);
    ADD_API_METHOD_1(getAttributeIndex);
    ADD_API_METHOD_0(asMidiPlayer);
}
```

### Dynamic Constants

Two sources of dynamic constants are registered at construction time:

1. **Parameter index constants:** For each parameter on the underlying `MidiProcessor`, a constant is added with the parameter's identifier name mapped to its integer index. E.g., if the MIDI processor has parameters "Intensity" (0) and "Speed" (1), then `mp.Intensity` resolves to `0` and `mp.Speed` resolves to `1`. These come from `mp->getIdentifierForParameterIndex(i)`.

2. **ScriptParameters object:** The `addScriptParameters()` helper (ScriptingApiObjects.cpp:175-188) checks if the underlying processor is a `ProcessorWithScriptingContent` (i.e., a script processor). If so, it creates a `DynamicObject` mapping each scripting content component name to its index. This is added as the constant `ScriptParameters`. For non-script MIDI processors, `ScriptParameters` is still added but empty.

```cpp
void addScriptParameters(ConstScriptingObject* this_, Processor* p)
{
    DynamicObject::Ptr scriptedParameters = new DynamicObject();
    if (ProcessorWithScriptingContent* pwsc = dynamic_cast<ProcessorWithScriptingContent*>(p))
    {
        for (int i = 0; i < pwsc->getScriptingContent()->getNumComponents(); i++)
        {
            scriptedParameters->setProperty(
                pwsc->getScriptingContent()->getComponent(i)->getName(), var(i));
        }
    }
    this_->addConstant("ScriptParameters", var(scriptedParameters.get()));
}
```

### Method Registration

ALL 14 methods use plain `ADD_API_METHOD_N` macros (no `ADD_TYPED_API_METHOD_N`). This means no forced parameter types -- all types must be inferred from context.

The Wrapper struct uses standard `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` macros.

## AssignableObject Interface

**File:** `ScriptingBaseObjects.h:612-651`

The `AssignableObject` interface enables bracket-based property access in HiseScript:

```javascript
// These are equivalent:
mp.setAttribute(mp.Intensity, 0.5);
mp["Intensity"] = 0.5;
```

### getCachedIndex (ScriptingApiObjects.cpp:4602-4614)

```cpp
int getCachedIndex(const var &indexExpression) const
{
    if (checkValidObject())
    {
        Identifier id(indexExpression.toString());
        for (int i = 0; i < mp->getNumParameters(); i++)
        {
            if (id == mp->getIdentifierForParameterIndex(i)) return i;
        }
    }
    return -1;
}
```

Converts a string expression to a parameter index by iterating parameter identifiers.

### assign (ScriptingApiObjects.cpp:4617-4620)

```cpp
void assign(const int index, var newValue)
{
    setAttribute(index, (float)newValue);
}
```

Delegates to `setAttribute()`.

### getAssignedValue (ScriptingApiObjects.cpp:4622-4625)

```cpp
var getAssignedValue(int /*index*/) const
{
    return 1.0; // Todo...
}
```

**Note:** This is a stub that always returns `1.0`. Reading via bracket syntax does NOT return the actual attribute value. This is a known incomplete implementation.

## Factory Methods / obtainedVia

### Synth.getMidiProcessor(name) -- PRIMARY

**File:** `ScriptingApi.cpp:5926-5958`

- **Restriction:** `onInit` only (enforced by `objectsCanBeCreated()`)
- **Search scope:** Owner-rooted subtree search using `Processor::Iterator<MidiProcessor>(owner)`
- **Self-exclusion:** Cannot get a reference to the script processor that owns the Synth object (`if(name == getProcessor()->getId())` -> error)
- Returns `new ScriptingMidiProcessor(getScriptProcessor(), mp)` on match
- Reports script error if not found

### MidiPlayer.asMidiProcessor() -- REVERSE CAST

**File:** `ScriptingApiObjects.cpp:6844-6852`

Creates a `ScriptingMidiProcessor` wrapping the MidiPlayer's underlying `MidiPlayer*` (which inherits from `MidiProcessor`). This enables using generic attribute get/set on a MidiPlayer module.

### Builder

The Builder class can also create/return MidiProcessor references. At line 6848 in ScriptingApiObjects.cpp, `ScriptingMidiProcessor` is constructed in Builder context. Line 10409 confirms type matching: `RETURN_IF_MATCH(ScriptingMidiProcessor, hise::MidiProcessor)`.

## Method Implementation Details

### setAttribute (ScriptingApiObjects.cpp:4635-4641)

```cpp
void setAttribute(int index, float value)
{
    if (checkValidObject())
    {
        mp->setAttribute(index, value, ProcessorHelpers::getAttributeNotificationType());
    }
}
```

Uses `ProcessorHelpers::getAttributeNotificationType()` for the notification flag. This is a global helper (Processor.h:1022) that returns the appropriate notification type based on context.

### getAttribute (ScriptingApiObjects.cpp:4643-4651)

Direct delegation to `mp->getAttribute(parameterIndex)`. Returns `0.0f` if invalid.

### getNumAttributes (ScriptingApiObjects.cpp:4653-4661)

Delegates to `mp->getNumParameters()`.

### getAttributeId / getAttributeIndex (ScriptingApiObjects.cpp:4663-4677)

`getAttributeId(index)` -> `mp->getIdentifierForParameterIndex(index).toString()`
`getAttributeIndex(id)` -> `mp->getParameterIndexForIdentifier(id)`

These are string<->int conversions for parameter identification.

### setBypassed (ScriptingApiObjects.cpp:4679-4686)

```cpp
void setBypassed(bool shouldBeBypassed)
{
    if (checkValidObject())
    {
        mp->setBypassed(shouldBeBypassed, sendNotification);
        mp->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Bypassed);
    }
}
```

Sends both a bypass notification and a `ProcessorChangeEvent::Bypassed` dispatch event.

### exportState / restoreState (ScriptingApiObjects.cpp:4698-4721)

`exportState()` uses `ProcessorHelpers::getBase64String(mp, false, false)` -- full processor state, not copied to clipboard, not content-only.

`restoreState()` validates the base64 string by parsing it into a ValueTree first, then calls `ProcessorHelpers::restoreFromBase64String(mp, base64State, false)`.

Signature from Processor.h:
```cpp
static String getBase64String(const Processor* p, bool copyToClipboard=true, bool exportContentOnly=false);
static void restoreFromBase64String(Processor* p, const String& base64String, bool restoreScriptContentOnly=false);
```

### exportScriptControls / restoreScriptControls (ScriptingApiObjects.cpp:4724-4750)

These methods have an additional guard: they check if the underlying processor is a `ProcessorWithScriptingContent` via dynamic_cast. If not, they report a script error: "exportScriptControls can only be used on Script Processors" / "restoreScriptControls can only be used on Script Processors".

When the guard passes:
- `exportScriptControls()` uses `getBase64String(mp, false, true)` -- the `true` flag means `exportContentOnly`, which exports only the scripting content (UI control values), not the full processor state.
- `restoreScriptControls()` uses `restoreFromBase64String(mp, base64Controls, true)` -- the `true` flag means `restoreScriptContentOnly`.

This allows saving/restoring just the UI control values of another script processor without triggering a full recompile.

### asMidiPlayer (ScriptingApiObjects.cpp:4752-4761)

```cpp
var asMidiPlayer()
{
    if (auto player = dynamic_cast<MidiPlayer*>(mp.get()))
    {
        return var(new ScriptedMidiPlayer(getScriptProcessor(), player));
    }
    reportScriptError("The module is not a MIDI player");
    RETURN_IF_NO_THROW(var());
}
```

Dynamic casts the underlying `MidiProcessor*` to `MidiPlayer*`. If the module is not a MidiPlayer, it reports a script error. This is the inverse of `MidiPlayer.asMidiProcessor()`.

## Relationship to Synth Prerequisite

Per the Synth Readme, `Synth.getMidiProcessor()` uses owner-rooted subtree search and is onInit-only. The MidiProcessor handle follows the same generic module handle pattern as Effect, Modulator, ChildSynth -- attribute get/set, bypass, state export/restore. The attribute system maps to the parent synth's module tree model described in the Synth analysis.

The self-exclusion check in `getMidiProcessor` is unique to this handle type -- you cannot get a reference to the script that owns the Synth object. This prevents circular references in script-to-script communication.

## Threading / Lifecycle Constraints

- **onInit only:** The `getMidiProcessor()` factory is restricted to `onInit` via `objectsCanBeCreated()`.
- **setAttribute notification:** Uses `ProcessorHelpers::getAttributeNotificationType()` which returns the appropriate notification type based on the calling context (async on audio thread, sync on message thread).
- **WeakReference safety:** All methods check object validity before proceeding. If the underlying processor is deleted, methods return defaults silently.
- **No audio-thread restrictions on methods themselves:** Once created, `setAttribute`, `getAttribute`, etc. can be called from any callback.

## Preprocessor Guards

None specific to this class. No `#if USE_BACKEND` or other conditional compilation guards affect the MidiProcessor scripting wrapper.

## What MidiProcessor Types Exist

The `hise::MidiProcessor` C++ base class is the parent for all MIDI processing modules. Common types obtained via `Synth.getMidiProcessor()`:

- **JavascriptMidiProcessor** -- script MIDI processors (these are ProcessorWithScriptingContent, so exportScriptControls works)
- **MidiPlayer** -- MIDI file player (can be cast to ScriptedMidiPlayer via asMidiPlayer())
- **Transposer, Arpeggiator, CC Remapper** -- built-in MIDI effect modules (no scripting content, exportScriptControls will error)

The parameter indices and identifiers vary per module type -- they are dynamically registered at construction time based on the actual processor instance.
