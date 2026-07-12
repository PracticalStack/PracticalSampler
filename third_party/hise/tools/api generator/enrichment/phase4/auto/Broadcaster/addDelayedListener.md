Adds a listener whose callback fires after a specified delay in milliseconds. Each new broadcast resets the delay timer, so only the most recent message within the delay window triggers the callback. This provides a debounce mechanism for rapid events.

The callback always receives the broadcaster's most recent values at the time the timer fires, not the values from the original send. Passing `0` for the delay falls back to `addListener` behaviour (no delay).
