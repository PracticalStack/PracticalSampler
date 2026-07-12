# Console -- Methods

## print

**Signature:** `undefined print(NotUndefined x)`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Prints a value to the HISE console. The value is converted to a string representation using `.toString()`. In the HISE IDE (backend builds), the value is also shown as an inline debug value in the code editor at the calling line. In exported plugins (frontend builds), this falls back to a debug-only `DBG()` macro, which is stripped in release builds -- effectively a no-op.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | NotUndefined | no | The value to print to the console. Any type is accepted; it will be converted to its string representation. | -- |

**Pitfalls:**
- In exported plugins, `Console.print()` produces no visible output. Do not rely on it for end-user logging.
- On the audio thread, the output is deferred to the message thread internally, but the call itself is permitted.

**Cross References:**
- `$API.Console.clear$`

**Example:**
```javascript
Console.print("Hello World");
Console.print(42);
Console.print({"key": "value"});
```

## startBenchmark

**Signature:** `undefined startBenchmark()`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Starts a high-resolution benchmark timer by recording the current timestamp. Call `Console.stopBenchmark()` to end the measurement and print the elapsed time in milliseconds to the console. Only one benchmark can be active at a time per Console instance; calling `startBenchmark()` again before stopping overwrites the previous start time.

**Parameters:**

None.

**Cross References:**
- `$API.Console.stopBenchmark$`

## stopBenchmark

**Signature:** `undefined stopBenchmark()`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Stops the benchmark timer started by `Console.startBenchmark()` and prints the elapsed time in milliseconds (to 3 decimal places) to the console. Reports a script error if `startBenchmark()` was not called first. Only produces output in the HISE IDE (backend builds).

**Parameters:**

None.

**Pitfalls:**
- Calling `stopBenchmark()` without a preceding `startBenchmark()` throws a script error.
- The result is only printed in backend builds; in exported plugins this is a no-op.

**Cross References:**
- `$API.Console.startBenchmark$`

**Example:**
```javascript
Console.startBenchmark();

for (var i = 0; i < 10000; i++)
    Math.sin(i * 0.01);

Console.stopBenchmark(); // Prints e.g. "Benchmark Result: 1.234 ms"
```

## stop

**Signature:** `undefined stop(Integer condition)`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
A cooperative breakpoint that halts script execution when `condition` is true. On the scripting thread, sample-loading thread, or audio thread, it suspends execution using the `JavascriptThreadPool::ScopedSleeper` mechanism, rebuilds debug information, and waits until the user resumes from the HISE IDE. On the message (UI) thread, it reports a script error instead of suspending because blocking the UI thread would freeze the application. The timeout is automatically extended by the duration of the suspension so the script does not time out while paused.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| condition | Integer | no | When true (non-zero), execution is suspended at this line. When false (0), this call is a no-op. | -- |

**Pitfalls:**
- Cannot be used on the message thread; doing so throws a script error ("Breakpoint in UI Thread").
- While paused, the audio thread outputs silence. This is by design -- the kill state handler suspends audio processing during the breakpoint.

**Cross References:**
- `$API.Console.breakInDebugger$`
- `$API.Console.blink$`

**Example:**
```javascript:basic
// Conditional breakpoint: only stop when value is out of range
Console.stop(myValue < 0);

// Unconditional breakpoint
Console.stop(true);
```

## blink

**Signature:** `undefined blink()`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Sends a visual blink (flash) message to the HISE code editor at the line where this call is located. Useful as a lightweight visual indicator that a particular code path has been reached, without halting execution. Only works in the HISE IDE when the new code editor is enabled (`HISE_USE_NEW_CODE_EDITOR`). The blink is dispatched asynchronously to the message thread.

**Parameters:**

None.

**Pitfalls:**
- Only works if the code editor currently displaying the file that contains the `blink()` call is active. If a different file is open, the blink is silently ignored.

## clear

**Signature:** `undefined clear()`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Clears all output in the HISE console.

**Parameters:**

None.

## assertTrue

**Signature:** `undefined assertTrue(NotUndefined condition)`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Throws a script error if `condition` evaluates to `false`. The value is cast to `bool`, so any falsy value (0, `false`, empty string, undefined) will trigger the assertion. The error message is a generic "Assertion failure: condition is false".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| condition | NotUndefined | no | The condition to check. Cast to `bool` -- falsy values trigger the assertion. | -- |

**Cross References:**
- `$API.Console.assertWithMessage$`
- `$API.Console.assertEqual$`

## assertEqual

**Signature:** `undefined assertEqual(NotUndefined v1, NotUndefined v2)`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Throws a script error if `v1` and `v2` are not equal. Uses the generic `!=` operator for comparison, so the values are compared by value for primitives and by reference for objects. The error message includes the string representation of both values.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| v1 | NotUndefined | no | The expected value. | -- |
| v2 | NotUndefined | no | The actual value to compare against `v1`. | -- |

**Cross References:**
- `$API.Console.assertTrue$`
- `$API.Console.assertWithMessage$`

**Example:**
```javascript
var result = calculateSomething();
Console.assertEqual(42, result); // Error if result != 42
```

## assertIsDefined

**Signature:** `undefined assertIsDefined(NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Throws a script error if `value` is `undefined` or void. This is useful for validating that a variable has been properly initialised or that a function returned a meaningful result.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | The value to check for definedness. Triggers the assertion if undefined or void. | -- |

**Cross References:**
- `$API.Console.assertTrue$`

## assertIsObjectOrArray

**Signature:** `undefined assertIsObjectOrArray(NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Throws a script error if `value` is not a JSON object or an array. The error message includes the actual type of the value. Useful for validating function parameters that expect structured data.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | The value to check. Must be an object or array, otherwise the assertion fails. | -- |

**Cross References:**
- `$API.Console.assertLegalNumber$`
- `$API.Console.assertNoString$`

## assertNoString

**Signature:** `undefined assertNoString(NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Throws a script error if `value` is a string. The error message includes the string value itself. This is typically used to catch accidental string-to-number coercion bugs, e.g. when a parameter that should be numeric has been inadvertently passed as a string.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | The value to check. Triggers the assertion if it is a string type. | -- |

**Pitfalls:**
- The error message is `"Assertion failure: " + value.toString()`, which means the string value itself becomes the error message body. This can be confusing if the string content looks like a different error.

**Cross References:**
- `$API.Console.assertLegalNumber$`
- `$API.Console.assertIsObjectOrArray$`

## assertWithMessage

**Signature:** `undefined assertWithMessage(Integer condition, String errorMessage)`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Throws a script error with the provided `errorMessage` if `condition` is false. This is the most flexible assertion method, allowing you to specify exactly what went wrong.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| condition | Integer | no | The condition to check. When false (0), the assertion fails. | -- |
| errorMessage | String | no | The error message to display on assertion failure, prefixed with "Assertion failure: ". | -- |

**Cross References:**
- `$API.Console.assertTrue$`

**Example:**
```javascript:basic
// Title: Custom assertion messages for debugging
// Context: When assertTrue would be cryptic, assertWithMessage provides
// context-specific error messages that include runtime values.

const var index = -5;

Console.assertWithMessage(index >= 0, "Index must not be negative, got: " + index);
```
```json:testMetadata:basic
{
  "testable": true,
  "verifyScript": {
    "type": "expect-error",
    "errorMessage": "Index must not be negative, got: -5"
  }
}
```


## assertLegalNumber

**Signature:** `undefined assertLegalNumber(NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Throws a script error if `value` is not a finite, legal number. This performs two checks: first it verifies the value is numeric (not a string, object, array, etc.), then it checks that the numeric value is not `NaN` or infinity using HISE's `FloatSanitizers`. The error message includes the type and/or value on failure.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | The value to validate. Must be a finite number (int or double). | -- |

**Cross References:**
- `$API.Console.assertNoString$`
- `$API.Console.assertIsObjectOrArray$`

**Example:**
```javascript:basic
// Example: Validating gain calculations before applying
// Context: Division operations can produce illegal numbers (infinity/NaN)
// that corrupt audio processing. assertLegalNumber catches these.

var inputLevel = 0.0;
var gain1 = 1.0 / inputLevel; // Division by zero creates infinity

Console.assertLegalNumber(gain1); // Assertion fires - infinity is illegal
```
```json:testMetadata:basic
{
  "testable": true,
  "verifyScript": {
    "type": "expect-error",
    "errorMessage": "value is not a legal number"
  }
}
```


## breakInDebugger

**Signature:** `undefined breakInDebugger()`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Triggers a native C++ assertion (`jassertfalse`) which will break into the attached C++ debugger (e.g. Visual Studio or Xcode). This is only useful for HISE developers who are running HISE from a C++ IDE with a debugger attached. For normal HISEScript debugging, use `Console.stop()` instead.

**Parameters:**

None.

**Pitfalls:**
- This breaks into the **C++ debugger**, not the HISEScript debugger. It has no effect if no native debugger is attached (the assertion is silently ignored in release builds).

**Cross References:**
- `$API.Console.stop$`

## startSampling

**Signature:** `undefined startSampling(String sessionId)`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Starts a data sampling session with the given identifier. Once a session is active, subsequent calls to `Console.sample()` will record data snapshots into the session. The session is managed by the profiling toolkit's `DebugSession` system. If successful, an inline debug value is shown in the code editor at the calling line. Requires `HISE_INCLUDE_PROFILING_TOOLKIT` to be enabled; otherwise this is a no-op.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sessionId | String | no | A unique identifier for the sampling session. | -- |

**Cross References:**
- `$API.Console.sample$`

**Example:**
```javascript:basic
Console.startSampling("myDebugSession");

// ... later, inside a callback or loop:
Console.sample("velocity", Message.getVelocity());
Console.sample("state", myStateObject);

Console.stopSampling("myDebugSession");
```

## testCallback

**Signature:** `undefined testCallback(ScriptObject obj, String callbackId, NotUndefined argList)`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Synchronously invokes a named callback on a UI component for automated testing. The `obj` parameter must be a reference to a ScriptComponent (e.g. a button, slider, or panel). The `callbackId` identifies which callback to trigger (e.g. the paint routine or a custom callback). The `argList` can be a single value or an array of arguments to pass to the callback. Diagnostic messages (`BEGIN_CALLBACK_TEST`, `END_CALLBACK_TEST`, `CALLBACK_ARGS`) are printed to the console. Reports a script error if the callback execution fails.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| obj | ScriptObject | no | A reference to a ScriptComponent whose callback will be tested. | Must be a valid UI component reference. |
| callbackId | String | no | The name of the callback to invoke on the component. | -- |
| argList | NotUndefined | no | Arguments to pass to the callback. Can be a single value or an array of values. | -- |

**Pitfalls:**
- Intended only for automated testing setups. A warning is logged if called outside a testing configuration (`isFlakyThreadingAllowed` is false).
- The component must support the specified callback; otherwise the internal `testCallback` will return an error.

**Example:**
```javascript
// Test a panel's paint callback with a Graphics object argument
Console.testCallback(myPanel, "paint", [g]);

// Test a button's custom callback with a single value
Console.testCallback(myButton, "action", 1);
```

## sample

**Signature:** `undefined sample(String label, NotUndefined dataToSample)`
**Return Type:** `undefined`
**Call Scope:** safe

**Description:**
Records a labelled data snapshot into the currently active sampling session. The data is cloned at the point of capture, so subsequent mutations do not affect the recorded value. If no session has been started via `Console.startSampling()`, a one-time warning is printed to the console and the call is skipped. An inline debug value is shown in the code editor at the calling line when a session is active. Requires `HISE_INCLUDE_PROFILING_TOOLKIT` to be enabled; otherwise this is a no-op.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| label | String | no | A descriptive label for this data sample point. | -- |
| dataToSample | NotUndefined | no | The data to record. The value is cloned at capture time. Any type is accepted. | -- |

**Pitfalls:**
- The "no session started" warning is only shown once per Console instance. Subsequent calls without a session are silently skipped.

**Cross References:**
- `$API.Console.startSampling$`
