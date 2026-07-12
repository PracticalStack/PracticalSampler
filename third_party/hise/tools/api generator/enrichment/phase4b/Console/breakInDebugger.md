Console::breakInDebugger() -> undefined

Thread safety: SAFE
Triggers a native C++ assertion (`jassertfalse`) which breaks into the attached C++ debugger (Visual Studio/Xcode). Only useful for HISE developers running from a C++ IDE. For normal HISEScript debugging, use `Console.stop()` instead.

Anti-patterns:
- This breaks into the C++ debugger, not the HISEScript debugger. No effect if no native debugger is attached; assertion is silently ignored in release builds.

Pair with: Console.stop -- cooperative HISEScript breakpoint for normal script debugging.
