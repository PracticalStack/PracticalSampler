Sets the component's value. Thread-safe - can be called from any thread; the UI update happens asynchronously. The value propagates to all linked component targets.

> [!Warning:String values cause script error] Do not pass a String value - this reports a script error. Use numeric values, arrays, or objects only.
