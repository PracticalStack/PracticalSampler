Halts script execution when `condition` is true, acting as a conditional breakpoint in the HISE IDE. Execution suspends until you resume from the debugger. Audio output goes silent while paused.

> [!Warning:Cannot use on message thread] Cannot be used on the message thread - doing so throws a script error. Only use from scripting, audio, or sample-loading threads.
