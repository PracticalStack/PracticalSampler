# Date -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` -- Date entry (standalone utility, no prerequisites, no seeAlso, no createdBy)
- `enrichment/base/Date.json` -- 4 API methods
- No prerequisite class (standalone)
- No base class exploration needed (inherits from ApiClass directly)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.h`, line 718

```cpp
/** This class takes over a few of the Engine methods in order to break down this gigantomanic object. */
class Date : public ApiClass,
             public ScriptingObject
{
public:
    Date(ProcessorWithScriptingContent* s);
    ~Date() {};

    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Date"); }

    // ================================================================================================== API Calls

    /** Returns a fully described string of this date and time in milliseconds or ISO-8601 format
        (using the local timezone) with or without divider characters. */
    String getSystemTimeISO8601(bool includeDividerCharacters);

    /** Returns the system time in milliseconds. */
    int64 getSystemTimeMs();

    /** Returns a time in milliseconds to a date string. */
    String millisecondsToISO8601(int64 miliseconds, bool includeDividerCharacters);

    /** Returns a date string to time in milliseconds. */
    int64 ISO8601ToMilliseconds(String iso8601);

    struct Wrapper;
};
```

### Inheritance

- **ApiClass** -- The HISE scripting API base class for namespace-style API objects. Provides function registration (`addFunction0..5`), constant registration (`addConstant`), and diagnostic infrastructure.
- **ScriptingObject** -- Provides access to the `ProcessorWithScriptingContent` context (the owning script processor and MainController).

### Category

This is a **namespace-style** API class (category: `"namespace"` in base JSON). It has no instance methods -- all methods are static utility functions accessed through the global `Date` object.

### Design Intent

The Doxygen comment says: "This class takes over a few of the Engine methods in order to break down this gigantomanic object." This means Date was factored out of the Engine class to reduce Engine's method count. The same pattern applies to `Settings`, `FileSystem`, `Threads`, and `Server`.

## Constructor

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, line 3745

```cpp
ScriptingApi::Date::Date(ProcessorWithScriptingContent* s) :
    ScriptingObject(s),
    ApiClass(0)  // 0 constants
{
    ADD_API_METHOD_1(getSystemTimeISO8601);
    ADD_API_METHOD_0(getSystemTimeMs);
    ADD_API_METHOD_2(millisecondsToISO8601);
    ADD_API_METHOD_1(ISO8601ToMilliseconds);
}
```

### Key Observations

- **`ApiClass(0)`** -- Zero constants registered. The Date class exposes no named constants (no enum values, no mode selectors).
- **No `ADD_TYPED_API_METHOD_N` calls** -- All methods use plain `ADD_API_METHOD_N`, meaning parameter types are not enforced at the scripting engine level. Types must be inferred from the C++ signatures.
- **No diagnostics** -- No `addDiagnostic()`, no `ADD_CALLBACK_DIAGNOSTIC`, no deprecated methods.

## Wrapper Struct

```cpp
struct ScriptingApi::Date::Wrapper
{
    API_METHOD_WRAPPER_1(Date, getSystemTimeISO8601);
    API_METHOD_WRAPPER_0(Date, getSystemTimeMs);
    API_METHOD_WRAPPER_2(Date, millisecondsToISO8601);
    API_METHOD_WRAPPER_1(Date, ISO8601ToMilliseconds);
};
```

All four methods use `API_METHOD_WRAPPER_N` (not `API_VOID_METHOD_WRAPPER_N`), confirming all methods return values. The wrapper pattern is `return var(static_cast<Date*>(m)->methodName(...))`, which auto-converts the C++ return type to a `var`.

## Method Implementations

All four methods are trivial wrappers around JUCE's `juce::Time` class.

### getSystemTimeISO8601

```cpp
String ScriptingApi::Date::getSystemTimeISO8601(bool includeDividerCharacters)
{
    return Time::getCurrentTime().toISO8601(includeDividerCharacters);
}
```

- `Time::getCurrentTime()` returns the current system time as a `juce::Time` object (using the local timezone).
- `toISO8601(bool)` formats the time as an ISO-8601 string. When `includeDividerCharacters` is true, the output uses dashes and colons (e.g., `2026-03-09T14:30:00`). When false, dividers are omitted (e.g., `20260309T143000`).

### getSystemTimeMs

```cpp
int64 ScriptingApi::Date::getSystemTimeMs()
{
    return Time::getCurrentTime().toMilliseconds();
}
```

- Returns the current system time as milliseconds since the Unix epoch (January 1, 1970 00:00:00 UTC).
- The `int64` return type is converted to a `var` by the wrapper, which preserves it as a 64-bit integer in HiseScript.

### millisecondsToISO8601

```cpp
String ScriptingApi::Date::millisecondsToISO8601(int64 miliseconds, bool includeDividerCharacters)
{
    return Time(miliseconds).toISO8601(includeDividerCharacters);
}
```

- Constructs a `juce::Time` from a millisecond epoch value and converts to ISO-8601 string.
- Note the parameter name has a typo: `miliseconds` (single 'l') -- this is in the C++ source and propagates to the API signature.

### ISO8601ToMilliseconds

```cpp
int64 ScriptingApi::Date::ISO8601ToMilliseconds(String iso8601)
{
    return juce::Time::fromISO8601(iso8601).toMilliseconds();
}
```

- Parses an ISO-8601 string into a `juce::Time` and returns the millisecond epoch value.
- `juce::Time::fromISO8601()` is a static factory method that parses ISO-8601 formatted strings.

## Registration / ObtainedVia

**File:** `HISE/hi_scripting/scripting/ScriptProcessorModules.cpp`, line 312

```cpp
scriptEngine->registerApiClass(new ScriptingApi::Date(this));
```

The Date object is registered as a **global API class** via `scriptEngine->registerApiClass()`. This makes it available as the global namespace `Date` in HiseScript (e.g., `Date.getSystemTimeMs()`).

It is also listed in the `getGlobalApiClasses()` function (`ScriptingApiObjects.cpp`, line 6976), confirming it is part of the canonical set of global API namespaces.

Registration happens in `JavascriptMidiProcessor::registerScriptApiClasses()`, which means Date is available in every HiseScript context (MIDI processor scripts, Interface scripts, etc.).

### Registration Context

Date is registered alongside these sibling classes that were similarly factored out of Engine:
- `Settings` (line 309)
- `FileSystem` (line 310)
- `Threads` (line 311)
- `Date` (line 312)

## Threading / Lifecycle

- **No threading constraints.** All four methods call into `juce::Time` static methods, which are thread-safe (they call OS system clock APIs).
- **No onInit-only restrictions.** Date methods can be called from any callback (onInit, onNoteOn, onControl, timer, paint, etc.).
- **No state.** The class holds no member state beyond what ApiClass and ScriptingObject provide. Each method call is independent and stateless.

## Preprocessor Guards

None. The Date class has no `#if USE_BACKEND`, `#if HISE_INCLUDE_LORIS`, or any other conditional compilation guards. It is available in all build targets (backend IDE, frontend exported plugins, project DLLs).

## Dependencies

The class depends solely on:
- `juce::Time` -- JUCE's time utility class (always available)
- `ApiClass` -- HISE scripting API registration infrastructure
- `ScriptingObject` -- HISE scripting object base providing processor context

No external libraries, no optional modules, no complex data dependencies.

## Upstream Data Providers

`juce::Time::getCurrentTime()` calls the OS system clock. The time is in the **local timezone** (as documented in the Doxygen comment for `getSystemTimeISO8601`). There is no HISE-specific clock, transport, or sample-position involvement. The millisecond values are Unix epoch milliseconds (ms since 1970-01-01 00:00:00 UTC), but the ISO-8601 string representation uses the system's local timezone.

## Summary of Observations

- This is one of the simplest classes in the entire HISE scripting API.
- 4 methods, 0 constants, no state, no threading concerns, no conditionals.
- All methods are thin wrappers around `juce::Time`.
- No typed parameter enforcement -- all use plain `ADD_API_METHOD_N`.
- The class exists purely for organizational reasons (factored out of Engine).
- The conversion methods form a bidirectional pair: `millisecondsToISO8601` and `ISO8601ToMilliseconds`.
- The "system time" methods provide the current wall-clock time, not DAW transport time (for that, see `TransportHandler`).
