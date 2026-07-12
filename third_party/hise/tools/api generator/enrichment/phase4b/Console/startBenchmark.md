Console::startBenchmark() -> undefined

Thread safety: SAFE
Starts a high-resolution benchmark timer. Call `Console.stopBenchmark()` to end the measurement and print elapsed time in ms. Only one benchmark active at a time -- calling again overwrites the previous start time.

Pair with: Console.stopBenchmark -- ends the measurement and prints the result.
