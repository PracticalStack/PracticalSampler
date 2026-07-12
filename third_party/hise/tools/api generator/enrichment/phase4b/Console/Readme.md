Console (namespace)

Debug utility providing console output, assertions, benchmarking, and data
sampling for HISEScript development. All methods are permitted on the audio
thread. Most output-producing methods are no-ops in exported plugin builds.

Complexity tiers:
  1. Basic debugging: print, clear. Logging values and resetting the console.
  2. Defensive assertions: assertTrue, assertIsDefined, assertIsObjectOrArray.
     Guard clauses in utility functions and module tree construction.
  3. Unit-test-style validation: + assertEqual, assertWithMessage,
     assertNoString, assertLegalNumber. Verifying computed values, descriptive
     failure messages, and type-safety checks in initialization routines.

Practical defaults:
  - Leave Console.print calls in production code freely -- they become no-ops
    in exported plugins, so there is no cost and they document control flow.
  - Use Console.assertTrue(false) as an "unreachable code" marker in functions
    that must return from a loop or switch. Clearer than a bare return.
  - Prefer Console.assertIsDefined(obj) over manual isDefined checks when the
    absence of a value is a programming error, not a normal state.
  - Use Console.clear() before multi-step operations (batch export, preset
    conversion) to isolate output from prior noise.

Common mistakes:
  - Calling stopBenchmark() without a preceding startBenchmark() -- throws a script error.
  - Relying on Console.print() output in exported plugins -- gated by USE_BACKEND, effectively a no-op in frontend builds.
  - Using Console.stop() on the message thread -- reports a script error instead of suspending execution.
  - Calling Console.sample() without Console.startSampling() -- logs a one-time warning and skips.
  - Wrapping Console.print() in if (Engine.isHISE()) guards -- Console methods are already no-ops in exported plugins, manual guards add clutter.
  - Using silent early returns instead of assertions when undefined is a bug -- assertIsDefined surfaces the error immediately during development.

Example:
  // Basic printing
  Console.print("Hello from HISEScript");

  // Benchmarking a block of code
  Console.startBenchmark();
  for (var i = 0; i < 1000; i++)
      Math.sin(i);
  Console.stopBenchmark();

  // Assertions for debugging
  Console.assertTrue(myValue > 0);
  Console.assertEqual(expected, actual);
  Console.assertIsDefined(someObject);
  Console.assertLegalNumber(gainValue);
  Console.assertIsObjectOrArray(data);
  Console.assertNoString(numericParam);
  Console.assertWithMessage(x > 0, "x must be positive");

  // Breakpoint - halts execution (scripting thread only)
  Console.stop(shouldBreak);

  // Blink the current line in the code editor
  Console.blink();

  // Clear the console output
  Console.clear();

  // Data sampling session (requires profiling toolkit)
  Console.startSampling("mySession");
  Console.sample("iteration", currentData);

Methods (17):
  assertEqual           assertIsDefined       assertIsObjectOrArray
  assertLegalNumber     assertNoString        assertWithMessage
  assertTrue            blink                 breakInDebugger
  clear                 print                 sample
  startBenchmark        startSampling         stop
  stopBenchmark         testCallback
