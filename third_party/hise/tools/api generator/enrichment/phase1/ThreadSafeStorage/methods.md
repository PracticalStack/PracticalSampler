# ThreadSafeStorage -- Method Documentation

## clear

**Signature:** `void clear()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires exclusive write lock via ScopedMultiWriteLock (delegates to store()).
**Minimal Example:** `{obj}.clear();`

**Description:**
Resets the storage to an empty (undefined) state. Delegates to `store()` with an undefined value, inheriting its write lock.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.ThreadSafeStorage.store$`

## load

**Signature:** `var load()`
**Return Type:** `NotUndefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires blocking shared read lock via ScopedReadLock. Use tryLoad() for audio-thread reads.
**Minimal Example:** `var data = {obj}.load();`

**Description:**
Returns the currently stored value. Acquires a shared read lock that blocks if a writer is active. Multiple concurrent readers are allowed. Returns undefined if nothing has been stored or after `clear()`.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Blocks the calling thread if a writer holds the lock. Do not call from the audio thread -- use `tryLoad()` instead.

**Cross References:**
- `$API.ThreadSafeStorage.tryLoad$`
- `$API.ThreadSafeStorage.store$`

## store

**Signature:** `void store(var dataToStore)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires exclusive write lock via ScopedMultiWriteLock.
**Minimal Example:** `{obj}.store([1, 2, 3]);`

**Description:**
Stores a value with reference semantics. Acquires an exclusive write lock and swaps the new value in. The old value is destroyed outside the lock for efficiency. For arrays and objects, the storage holds a reference to the same underlying data -- mutations by the caller after storing also affect the stored copy. Use `storeWithCopy()` to break reference sharing.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dataToStore | NotUndefined | no | The value to store | Any type |

**Pitfalls:**
- Arrays and objects are stored by reference, not by value. If the caller modifies the original array after calling `store()`, the stored data changes too, creating a potential race condition with readers on other threads. Use `storeWithCopy()` when the caller will continue to modify the data.

**Cross References:**
- `$API.ThreadSafeStorage.storeWithCopy$`
- `$API.ThreadSafeStorage.clear$`
- `$API.ThreadSafeStorage.load$`
- `$API.ThreadSafeStorage.tryLoad$`

## storeWithCopy

**Signature:** `void storeWithCopy(var dataToStore)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a deep copy via clone(), then acquires exclusive write lock via store().
**Minimal Example:** `{obj}.storeWithCopy(myArray);`

**Description:**
Stores a deep copy of the value, breaking reference semantics. For arrays and objects, calls `clone()` to create an independent copy before delegating to `store()`. The caller can safely modify the original after this call without affecting the stored data. For primitive types (numbers, booleans), behaves identically to `store()` since these are already value types.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dataToStore | NotUndefined | no | The value to deep-copy and store | Any type |

**Pitfalls:**
- [BUG] String values are stored as empty strings. The implementation calls `copy.toString()` on a default-constructed (undefined) local variable instead of `dataToStore.toString()`, producing an empty string regardless of input. Use `store()` for string values as a workaround.

**Cross References:**
- `$API.ThreadSafeStorage.store$`
- `$API.ThreadSafeStorage.clear$`
- `$API.ThreadSafeStorage.load$`
- `$API.ThreadSafeStorage.tryLoad$`

## tryLoad

**Signature:** `var tryLoad(var returnValueIfLocked)`
**Return Type:** `NotUndefined`
**Call Scope:** safe
**Call Scope Note:** Non-blocking. Uses try_lock_shared() which returns immediately if the lock is unavailable.
**Minimal Example:** `var data = {obj}.tryLoad(-1);`

**Description:**
Non-blocking read for audio-thread use. Attempts to acquire a shared read lock without blocking. If successful, returns the stored value. If a writer holds the lock, returns `returnValueIfLocked` instead. This is the recommended read path for realtime callbacks.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| returnValueIfLocked | NotUndefined | no | Fallback value returned when the lock cannot be acquired | Should match the expected data type |

**Pitfalls:**
- The returned value may be the fallback even when data has been stored, if a concurrent write is in progress. Design the fallback to be a valid default for the consumer (e.g., 0.0 for a gain value, an empty array for a data list).

**Cross References:**
- `$API.ThreadSafeStorage.load$`
- `$API.ThreadSafeStorage.store$`

**Example:**
```javascript:tryload-fallback-pattern
// Title: Non-blocking read with fallback value
const var storage = Engine.createThreadSafeStorage();
storage.store([1, 2, 3]);

// tryLoad returns stored data when no write is in progress
var result = storage.tryLoad([]);
Console.print("Length: " + result.length); // Length: 3
```
```json:testMetadata:tryload-fallback-pattern
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "result.length", "value": 3},
    {"type": "REPL", "expression": "result[0]", "value": 1}
  ]
}
```
