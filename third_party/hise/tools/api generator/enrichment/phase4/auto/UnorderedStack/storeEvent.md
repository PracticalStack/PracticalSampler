Event-mode only. Copies the event at the given index into the provided MessageHolder without modifying the stack. Use this to inspect events by position, or as part of a drain loop that processes and removes all events.

> [!Warning:Drain from index 0 in while loop] When draining the stack, always read and remove from index 0 in a `while (!stack.isEmpty())` loop. A forward-iterating for loop skips elements because `removeElement()` swaps the last element into the vacated slot.
