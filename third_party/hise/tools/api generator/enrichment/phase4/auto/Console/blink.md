Sends a visual flash to the HISE code editor at the line where this call is located. Useful as a lightweight indicator that a code path has been reached, without halting execution.

> [!Warning:Only works with active code editor] Only works if the code editor displaying the file containing the `blink()` call is currently active. If a different file is open, the blink is silently ignored.
