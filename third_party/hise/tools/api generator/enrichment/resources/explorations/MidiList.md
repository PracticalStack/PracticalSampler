# MidiList Exploration (Phase 1 Step A1)

## Source Files

- **Header:** `hi_scripting/scripting/api/ScriptingApiObjects.h` (lines 251-320)
- **Implementation:** `hi_scripting/scripting/api/ScriptingApiObjects.cpp` (lines 37-173)
- **Base classes:** `hi_scripting/scripting/api/ScriptingBaseObjects.h`
  - `ConstScriptingObject` (line 118)
  - `AssignableObject` (line 612)
- **Factory method:** `hi_scripting/scripting/api/ScriptingApi.cpp` (line 3460)
- **API method macros:** `hi_scripting/scripting/api/ScriptMacroDefinitions.h`
- **VarTypeChecker:** `hi_scripting/scripting/engine/JavascriptApiClass.h` (line 90)

## Class Declaration (Full Header)

```cpp
// hi_scripting/scripting/api/ScriptingApiObjects.h:251-320

class MidiList : public ConstScriptingObject,
                 public AssignableObject
{
public:

    // ============================================================================================================

    MidiList(ProcessorWithScriptingContent *p);
    ~MidiList() {};

    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("MidiList"); }

    void assign(const int index, var newValue);
    int getCachedIndex(const var &indexExpression) const override;
    var getAssignedValue(int index) const override;

    int getNumChildElements() const override { return 128; }

    DebugInformationBase* getChildElement(int index) override;
    // ================================================================================================ API METHODS

    /** Fills the MidiList with a number specified with valueToFill. */
    void fill(int valueToFill);;

    /** Clears the MidiList to -1. */
    void clear();

    /** Returns the value at the given number. */
    int getValue(int index) const;

    /** Returns the number of occurences of 'valueToCheck' */
    int getValueAmount(int valueToCheck);;

    /** Returns the first index that contains this value. */
    int getIndex(int value) const;

    /** Checks if the list contains any data. */
    bool isEmpty() const { return numValues == 0; }

    /** Returns the number of values that are not -1. */
    int getNumSetValues() const { return numValues; }

    /** Sets the number to something between -127 and 128. */
    void setValue(int index, int value);;

    /** Sets a range of items to the same value. */
    void setRange(int startIndex, int numToFill, int value);

    /** Encodes all values into a base64 encoded string for storage. */
    String getBase64String() const;

    /** Restore the values from a String that was created with getBase64String(). */
    void restoreFromBase64String(String base64encodedValues);

    // ============================================================================================================

    struct Wrapper;

    const int* getRawDataPointer() const { return data; }

private:

    int data[128];
    int numValues = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiList);
    JUCE_DECLARE_WEAK_REFERENCEABLE(MidiList);

    // ============================================================================================================
};
```

## Inheritance Chain

### ConstScriptingObject (hi_scripting/scripting/api/ScriptingBaseObjects.h:118)

```cpp
class ConstScriptingObject : public ScriptingObject,
                             public ApiClass
{
public:
    ConstScriptingObject(ProcessorWithScriptingContent* p, int numConstants);

    virtual Identifier getObjectName() const = 0;
    Identifier getInstanceName() const override;

    virtual bool objectDeleted() const;
    virtual bool objectExists() const;
    virtual bool addLocationForFunctionCall(const Identifier& id,
                                            const DebugableObjectBase::Location& location);
    bool checkValidObject() const;
    void setName(const Identifier &name_) noexcept;
    void gotoLocationWithDatabaseLookup();

    // Diagnostic prototype factory (static)
    static ReferenceCountedObjectPtr<ConstScriptingObject> createDiagnosticPrototype(
        const Identifier& className, ProcessorWithScriptingContent* pwsc);

private:
    Identifier name;
    // ...
};
```

MidiList passes `0` to the `numConstants` parameter: `ConstScriptingObject(p, 0)`. This means no constants are registered.

### AssignableObject (hi_scripting/scripting/api/ScriptingBaseObjects.h:612)

```cpp
class AssignableObject
{
public:
    struct IndexedValue
    {
        IndexedValue(AssignableObject* obj_, int idx): index(idx), obj(obj_) {}

        var operator()()
        {
            if (obj.get() != nullptr)
                return obj->getAssignedValue(index);
            return var();
        }

        Identifier getId() const
        {
            String s = "%PARENT%";
            s << "[" << String(index) << "]";
            return Identifier(s);
        }

    private:
        const int index;
        WeakReference<AssignableObject> obj;
    };

    virtual ~AssignableObject() {};
    virtual void assign(const int index, var newValue) = 0;
    virtual var getAssignedValue(int index) const = 0;
    virtual int getCachedIndex(const var &indexExpression) const = 0;

    JUCE_DECLARE_WEAK_REFERENCEABLE(AssignableObject);
};
```

MidiList implements all three pure virtual methods:

```cpp
void ScriptingObjects::MidiList::assign(const int index, var newValue)           { setValue(index, (int)newValue); }
int ScriptingObjects::MidiList::getCachedIndex(const var &indexExpression) const  { return (int)indexExpression; }
var ScriptingObjects::MidiList::getAssignedValue(int index) const                { return getValue(index); }
```

This enables bracket-syntax access in HISEScript: `list[60] = 100` calls `assign(60, 100)` which delegates to `setValue(60, 100)`. `var v = list[60]` calls `getAssignedValue(60)` which delegates to `getValue(60)`.

## Constructor -- API Method Registration

```cpp
// hi_scripting/scripting/api/ScriptingApiObjects.cpp:54-70

ScriptingObjects::MidiList::MidiList(ProcessorWithScriptingContent *p) :
ConstScriptingObject(p, 0)   // <-- zero constants
{
    ADD_TYPED_API_METHOD_1(fill, VarTypeChecker::Number);
    ADD_API_METHOD_0(clear);
    ADD_API_METHOD_1(getValue);
    ADD_API_METHOD_1(getValueAmount);
    ADD_TYPED_API_METHOD_1(getIndex, VarTypeChecker::Number);
    ADD_API_METHOD_0(isEmpty);
    ADD_API_METHOD_3(setRange);
    ADD_API_METHOD_0(getNumSetValues);
    ADD_API_METHOD_2(setValue);
    ADD_TYPED_API_METHOD_1(restoreFromBase64String, VarTypeChecker::String);
    ADD_API_METHOD_0(getBase64String);

    clear();   // <-- initializes all 128 slots to -1, numValues = 0
}
```

### Registered API Methods

| Method | Macro | Type checking |
|--------|-------|---------------|
| fill | ADD_TYPED_API_METHOD_1 | `VarTypeChecker::Number` |
| clear | ADD_API_METHOD_0 | none |
| getValue | ADD_API_METHOD_1 | none |
| getValueAmount | ADD_API_METHOD_1 | none |
| getIndex | ADD_TYPED_API_METHOD_1 | `VarTypeChecker::Number` |
| isEmpty | ADD_API_METHOD_0 | none |
| setRange | ADD_API_METHOD_3 | none |
| getNumSetValues | ADD_API_METHOD_0 | none |
| setValue | ADD_API_METHOD_2 | none |
| restoreFromBase64String | ADD_TYPED_API_METHOD_1 | `VarTypeChecker::String` |
| getBase64String | ADD_API_METHOD_0 | none |

Total: 11 API methods. Three have forced type checking via `ADD_TYPED_API_METHOD_1`:
- `fill` forces `Number` (int or double)
- `getIndex` forces `Number`
- `restoreFromBase64String` forces `String`

### ADD_TYPED_API_METHOD_1 Macro

```cpp
// hi_scripting/scripting/api/ScriptMacroDefinitions.h:57
#define ADD_TYPED_API_METHOD_1(name, t1) \
    static const Identifier name ## _id (#name); \
    addFunction1(name ## _id, &Wrapper::name); \
    addForcedParameterTypes(name ## _id, VarTypeChecker::createParameterTypes(t1));
```

When HISE_SCRIPT_WATCHDOG is not defined, falls back to `ADD_API_METHOD_1(name)` (no type checking).

## Wrapper Struct

```cpp
// hi_scripting/scripting/api/ScriptingApiObjects.cpp:39-52

struct ScriptingObjects::MidiList::Wrapper
{
    API_VOID_METHOD_WRAPPER_1(MidiList, fill);
    API_VOID_METHOD_WRAPPER_0(MidiList, clear);
    API_METHOD_WRAPPER_1(MidiList, getValue);
    API_METHOD_WRAPPER_1(MidiList, getValueAmount);
    API_METHOD_WRAPPER_1(MidiList, getIndex);
    API_METHOD_WRAPPER_0(MidiList, isEmpty);
    API_METHOD_WRAPPER_0(MidiList, getNumSetValues);
    API_VOID_METHOD_WRAPPER_3(MidiList, setRange);
    API_VOID_METHOD_WRAPPER_2(MidiList, setValue);
    API_VOID_METHOD_WRAPPER_1(MidiList, restoreFromBase64String);
    API_METHOD_WRAPPER_0(MidiList, getBase64String);
};
```

Standard HISE pattern: `API_VOID_METHOD_WRAPPER_N` for void-returning methods, `API_METHOD_WRAPPER_N` for value-returning methods. The wrapper forwards scripting calls to the C++ method.

## Factory Method

```cpp
// hi_scripting/scripting/api/ScriptingApi.cpp:3460
ScriptingObjects::MidiList *ScriptingApi::Engine::createMidiList()
{
    return new ScriptingObjects::MidiList(getScriptProcessor());
}
```

Created via `Engine.createMidiList()`. The Engine class registers this as `ADD_API_METHOD_0(createMidiList)` (line 1373). No parameters. Returns a new MidiList instance with all 128 slots initialized to -1.

## Full Method Implementations

```cpp
// hi_scripting/scripting/api/ScriptingApiObjects.cpp:82-173

void ScriptingObjects::MidiList::fill(int valueToFill)
{
    for (int i = 0; i < 128; i++)
        data[i] = valueToFill;

    numValues = (int)(valueToFill != -1) * 128;
}

void ScriptingObjects::MidiList::clear()
{
    fill(-1);
}

int ScriptingObjects::MidiList::getValue(int index) const
{
    if (isPositiveAndBelow(index, 128))
        return (int)data[index];

    return -1;
}

int ScriptingObjects::MidiList::getValueAmount(int valueToCheck)
{
    if (isEmpty())
        return (int)(valueToCheck == -1) * 128;

    int amount = 0;

    for (int i = 0; i < 128; i++)
        amount += (int)(data[i] == valueToCheck);

    return amount;
}

int ScriptingObjects::MidiList::getIndex(int value) const
{
    if (isEmpty())
        return -1;

    for (int i = 0; i < 128; i++)
    {
        if (data[i] == value)
            return i;
    }

    return -1;
}

void ScriptingObjects::MidiList::setValue(int index, int value)
{
    if (isPositiveAndBelow(index, 128))
    {
        auto isClearing = value == -1;
        auto elementIsClear = data[index] == -1;
        auto doSomething = isClearing != elementIsClear;
        numValues += (int)doSomething * ((int)elementIsClear * 2 - 1);
        data[index] = value;
    }
}

void ScriptingObjects::MidiList::setRange(int startIndex, int numToFill, int value)
{
    startIndex = jlimit(0, 127, startIndex);
    numToFill = jmin(numToFill, 127 - startIndex);

    bool isClearing = value == -1;
    int delta = 0;

    for (int i = startIndex; i < numToFill; i++)
    {
        auto elementIsClear = data[i] == -1;
        auto doSomething = isClearing != elementIsClear;
        delta += (int)doSomething * ((int)elementIsClear * 2 - 1);

        data[i] = value;
    }

    numValues += delta;
}

String ScriptingObjects::MidiList::getBase64String() const
{
    MemoryOutputStream stream;
    Base64::convertToBase64(stream, data, sizeof(int) * 128);
    return stream.toString();
}

void ScriptingObjects::MidiList::restoreFromBase64String(String base64encodedValues)
{
    MemoryOutputStream stream(data, sizeof(int) * 128);
    Base64::convertFromBase64(stream, base64encodedValues);
}
```

## Debug Support

```cpp
// hi_scripting/scripting/api/ScriptingApiObjects.cpp:76-80

DebugInformationBase* ScriptingObjects::MidiList::getChildElement(int index)
{
    IndexedValue i(this, index);
    return new LambdaValueInformation(i, i.getId(), {}, DebugInformation::Type::Constant, getLocation());
}
```

`getNumChildElements()` returns 128 (header line 267). This means the HISE debugger can expand a MidiList variable to show all 128 slots as child elements.

## Internal Storage

- **data:** `int data[128]` -- plain fixed-size C array. Not heap-allocated. Stores arbitrary integers (not clamped to any range).
- **numValues:** `int numValues = 0` -- tracks how many slots contain a value other than the sentinel `-1`. Updated branchlessly in `setValue`, `setRange`, and `fill`.
- **Sentinel value:** `-1` indicates an "unset" slot. Used by `isEmpty()`, `getNumSetValues()`, `getIndex()`, and `getValueAmount()` to determine occupancy.

## Threading Constraints

- MidiList does NOT override `allowIllegalCallsOnAudioThread()` -- uses the default from `ConstScriptingObject`, which returns `false`. This means calling MidiList methods from the audio thread in contexts that check this flag will produce a warning.
- However, all MidiList operations are inherently branchless and lock-free (plain array reads/writes, integer arithmetic). No allocations, no locks, no I/O. The data is a stack-allocated `int[128]`.
- `getBase64String()` allocates a `MemoryOutputStream` (heap allocation) -- not safe on the audio thread.
- `restoreFromBase64String()` writes into the existing `data` array via `MemoryOutputStream(data, sizeof(int) * 128)` -- uses a pre-allocated buffer, but `Base64::convertFromBase64` may allocate internally.

## Preprocessor Guards

None. MidiList has no `#if USE_BACKEND`, `#if HISE_INCLUDE_*`, or other conditional compilation guards. The entire class is always compiled in all build targets (backend, frontend, project DLL).

## Known Bugs (Logged in enrichment/issues.md)

### 1. setRange loop bound bug

```cpp
void ScriptingObjects::MidiList::setRange(int startIndex, int numToFill, int value)
{
    startIndex = jlimit(0, 127, startIndex);
    numToFill = jmin(numToFill, 127 - startIndex);

    // BUG: loop uses `i < numToFill` but should use `i < startIndex + numToFill`
    for (int i = startIndex; i < numToFill; i++)
    {
        // ...
    }
}
```

When `startIndex > 0`, the loop condition `i < numToFill` can terminate before reaching the intended range, or not execute at all if `startIndex >= numToFill`. For example, `setRange(64, 10, 42)` clamps `numToFill` to `min(10, 63) = 10`, then loops `i = 64; i < 10` which never executes. The correct condition should be `i < startIndex + numToFill`.

### 2. restoreFromBase64String does not recalculate numValues

```cpp
void ScriptingObjects::MidiList::restoreFromBase64String(String base64encodedValues)
{
    MemoryOutputStream stream(data, sizeof(int) * 128);
    Base64::convertFromBase64(stream, base64encodedValues);
    // numValues is NOT recalculated here
}
```

After deserialization, `numValues` retains whatever value it had before the restore. If the deserialized data has a different number of non-sentinel slots, `isEmpty()`, `getNumSetValues()`, and the fast-path in `getValueAmount()`/`getIndex()` will return incorrect results.

### 3. setValue Doxygen comment is misleading

```cpp
/** Sets the number to something between -127 and 128. */
void setValue(int index, int value);
```

The implementation stores any `int` value with no clamping. The comment's claim of a `-127` to `128` range is incorrect.
