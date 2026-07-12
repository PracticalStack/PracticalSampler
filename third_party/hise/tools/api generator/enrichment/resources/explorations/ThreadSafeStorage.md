# ThreadSafeStorage -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` (ThreadSafeStorage entry)
- `enrichment/base/ThreadSafeStorage.json` (5 methods)
- No prerequisites required (standalone container class)
- No base class exploration needed (direct ConstScriptingObject child)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h`, lines 712-750

```cpp
class ScriptThreadSafeStorage: public ConstScriptingObject
{
public:
    ScriptThreadSafeStorage(ProcessorWithScriptingContent* pwsc);
    ~ScriptThreadSafeStorage() override;

    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("ThreadSafeStorage"); }

    void clear();
    void store(var dataToStore);
    void storeWithCopy(var dataToStore);
    var load();
    var tryLoad(var returnValueIfLocked);

private:
    hise::SimpleReadWriteLock lock;
    var data;

    struct Wrapper
    {
        API_VOID_METHOD_WRAPPER_0(ScriptThreadSafeStorage, clear);
        API_VOID_METHOD_WRAPPER_1(ScriptThreadSafeStorage, store);
        API_VOID_METHOD_WRAPPER_1(ScriptThreadSafeStorage, storeWithCopy);
        API_METHOD_WRAPPER_0(ScriptThreadSafeStorage, load);
        API_METHOD_WRAPPER_1(ScriptThreadSafeStorage, tryLoad);
    };
};
```

**Inheritance:** `ConstScriptingObject` (0 constants in constructor -- `ConstScriptingObject(pwsc, 0)`)

**Internal state:** Two members only:
- `hise::SimpleReadWriteLock lock` -- the synchronization primitive
- `var data` -- the stored value (any JUCE `var` type)

## Constructor

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, lines 8217-8225

```cpp
ScriptingObjects::ScriptThreadSafeStorage::ScriptThreadSafeStorage(ProcessorWithScriptingContent* pwsc):
    ConstScriptingObject(pwsc, 0)
{
    ADD_API_METHOD_0(clear);
    ADD_API_METHOD_1(store);
    ADD_API_METHOD_1(storeWithCopy);
    ADD_API_METHOD_0(load);
    ADD_API_METHOD_1(tryLoad);
}
```

- **No constants:** `ConstScriptingObject(pwsc, 0)` -- zero constants registered
- **No typed methods:** All use plain `ADD_API_METHOD_N`, not `ADD_TYPED_API_METHOD_N`
- **No addConstant() calls**

## Destructor

```cpp
ScriptingObjects::ScriptThreadSafeStorage::~ScriptThreadSafeStorage()
{
    clear();
}
```

Calls `clear()` on destruction, which acquires the write lock and sets `data` to `var()` (undefined). This ensures any thread waiting on a read lock is unblocked before the object is destroyed.

## Factory Method / obtainedVia

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, lines 2174-2177

```cpp
var ScriptingApi::Engine::createThreadSafeStorage()
{
    return var(new ScriptingObjects::ScriptThreadSafeStorage(getScriptProcessor()));
}
```

Created via `Engine.createThreadSafeStorage()`. No parameters. Returns a new instance each call.

## Method Implementations

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, lines 8232-8269

### clear() -- line 8232

```cpp
void ScriptingObjects::ScriptThreadSafeStorage::clear()
{
    store(var());
}
```

Delegates to `store()` with an undefined var. Inherits the write lock from `store()`.

### store(var dataToStore) -- line 8237

```cpp
void ScriptingObjects::ScriptThreadSafeStorage::store(var dataToStore)
{
    SimpleReadWriteLock::ScopedMultiWriteLock sl(lock);
    std::swap(data, dataToStore);
}
```

Uses `ScopedMultiWriteLock` (not `ScopedWriteLock`), meaning multiple writer threads are supported. The `std::swap` is efficient -- it moves the old data out and the new data in without copying. The old data (now in `dataToStore`) is destroyed when the local variable goes out of scope, AFTER the lock is released (since `sl` destructor runs before `dataToStore` destructor in reverse order... actually both are on the stack, but `sl` is declared first, so `dataToStore` destructor runs first, then `sl` destructor). Wait -- `sl` is declared first on line 8239, `dataToStore` is a parameter so it's already constructed. The destruction order is: locals in reverse order of declaration. `sl` is the only local. `dataToStore` is a by-value parameter, destroyed after the function returns. So:
1. `sl` destructor runs (releases lock)
2. Function returns
3. `dataToStore` (now holding the old data) is destroyed by the caller

This means the old data's destruction happens OUTSIDE the lock, which is correct -- avoids holding the lock during potentially expensive destructor operations.

### storeWithCopy(var dataToStore) -- line 8243

```cpp
void ScriptingObjects::ScriptThreadSafeStorage::storeWithCopy(var dataToStore)
{
    var copy;

    if(dataToStore.isString())
        copy = var(copy.toString());  // NOTE: likely bug -- should be dataToStore.toString()
    else
        copy = dataToStore.clone();

    store(copy);
}
```

**Potential bug on line 8248:** `copy.toString()` is called instead of `dataToStore.toString()`. Since `copy` was just default-constructed as `var()`, `copy.toString()` returns an empty string. This means `storeWithCopy` on a string value would store an empty string instead of a copy of the input string.

The `clone()` path for non-string types works correctly -- `var::clone()` creates a deep copy of arrays and objects.

The purpose of this method is to break reference semantics: since JUCE `var` uses reference counting for arrays/objects, storing via `store()` means the caller and the storage share the same underlying data. `storeWithCopy` creates an independent copy first.

### load() -- line 8255

```cpp
var ScriptingObjects::ScriptThreadSafeStorage::load()
{
    SimpleReadWriteLock::ScopedReadLock sl(lock);
    return data;
}
```

Acquires a shared read lock. Multiple readers can load simultaneously. Blocks if a writer holds the lock.

### tryLoad(var returnValueIfLocked) -- line 8261

```cpp
var ScriptingObjects::ScriptThreadSafeStorage::tryLoad(var returnValueIfLocked)
{
    if(auto sl = SimpleReadWriteLock::ScopedTryReadLock(lock))
    {
        return data;
    }

    return returnValueIfLocked;
}
```

Non-blocking read attempt. Uses `ScopedTryReadLock` which has `operator bool()`. If the lock cannot be acquired (a writer is active), returns the caller-provided default value instead of blocking. This is the audio-thread-safe read path.

## SimpleReadWriteLock Infrastructure

**File:** `HISE/hi_tools/hi_tools/MiscToolClasses.h`, lines 1158-1418

The lock used by ThreadSafeStorage is HISE's custom `SimpleReadWriteLock`, not `std::shared_mutex` (though `std::shared_mutex` is available as a commented-out alternative).

### Lock Type

```cpp
using LockType = audio_spin_mutex_shared;
```

Uses `audio_spin_mutex_shared` -- an audio-optimized spin mutex with shared (read) locking support.

### Key Lock Variants Used by ThreadSafeStorage

1. **`ScopedMultiWriteLock`** (used by `store()`/`clear()`):
   - Acquires exclusive mutex lock via `lock.mutex.lock()`
   - Supports multiple writer threads (asserts if same thread re-enters -- use `ScopedWriteLock` for reentrant writes)
   - Stores writer thread ID atomically

2. **`ScopedReadLock`** (used by `load()`):
   - Acquires shared mutex lock via `mutex.lock_shared()`
   - Blocks if a writer holds the lock
   - Allows concurrent readers
   - Skips locking if the calling thread IS the writer (avoids deadlock on same-thread read-after-write)

3. **`ScopedTryReadLock`** (used by `tryLoad()`):
   - Non-blocking: `mutex.try_lock_shared()`
   - Returns `false` via `operator bool()` if lock cannot be acquired
   - Also returns `true` if the calling thread is the writer (safe reentrant read)

### Lock Members

```cpp
LockType mutex;                              // audio_spin_mutex_shared
std::atomic<std::thread::id> writer = {};    // tracks current writer thread
bool enabled = true;                         // can be disabled via ScopedDisabler
bool fakeWriteLock = false;                  // for tryToAcquireLock=false paths
```

## Threading Model

ThreadSafeStorage is designed for the specific HISE threading pattern:

- **UI/scripting thread writes** data (via `store()` or `storeWithCopy()`)
- **Audio thread reads** data (via `tryLoad()` -- non-blocking, returns default if locked)
- **Other threads read** data (via `load()` -- blocking read)

The `tryLoad()` method is the critical audio-thread-safe path. It never blocks, making it safe to call from the audio callback. If a write is in progress, the audio thread gets the fallback value and continues without stalling.

The `store()` method uses `ScopedMultiWriteLock`, allowing writes from multiple non-audio threads (e.g., both UI thread and loading thread can write).

## Survey Data

From `class_survey_data.json`:
- **Domain:** data
- **Role:** container
- **callbackDensity:** 0.0 (no callbacks)
- **statefulness:** 1.0 (fully stateful)
- **threadingExposure:** 1.0 (maximum threading concern)
- **fanOut/fanIn:** 0.0/0.0 (standalone, no creates/createdBy)

### seeAlso Distinctions

| Related Class | Distinction |
|---------------|-------------|
| Array | Use Array for standard single-thread collections; use ThreadSafeStorage when data must be shared between audio and UI threads. |
| UnorderedStack | Use UnorderedStack for audio-thread-safe float/event tracking with insert/remove; use ThreadSafeStorage for passing arbitrary data objects between threads. |
| Threads | ThreadSafeStorage provides a lock-based container for passing data between threads; Threads provides utilities for querying which thread you are on and executing functions on the loading thread. |
| BackgroundTask | ThreadSafeStorage is a passive container for cross-thread data sharing; BackgroundTask actively runs code on a background thread with progress reporting -- use ThreadSafeStorage to pass results back. |

## Preprocessor Guards

None. ThreadSafeStorage has no conditional compilation. Available in all build targets (backend, frontend, DLL).

## store() vs storeWithCopy() Semantics

This is the key design decision in the class. JUCE `var` has reference semantics for complex types:

- **`store(myArray)`**: The storage holds a reference to the SAME array object. If the caller later modifies `myArray`, the stored data changes too (race condition risk).
- **`storeWithCopy(myArray)`**: The storage holds an independent deep copy. The caller can freely modify the original without affecting stored data.

For primitive types (int, double, bool, string), both methods are effectively equivalent since `var` copies these by value.

The `clone()` method on `var` performs deep copying for Arrays (recursively clones elements) and DynamicObjects.
