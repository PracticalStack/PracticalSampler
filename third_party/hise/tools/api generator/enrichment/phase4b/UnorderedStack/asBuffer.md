UnorderedStack::asBuffer(int getAllElements) -> Buffer

Thread safety: SAFE
Returns a live Buffer view of the underlying float array without copying.
getAllElements=true returns all 128 backing slots; false returns only occupied elements.

Dispatch/mechanics:
  getAllElements=true -> returns wholeBf (always 128 elements)
  getAllElements=false -> returns elementBuffer (resized on every mutation via updateElementBuffer)
  Both are live views -- modifying the buffer modifies the stack directly.

Pair with:
  insert/remove -- mutations automatically update the elementBuffer view size

Anti-patterns:
  - Do NOT call on an event-mode stack -- reports a script error
  - Do NOT use asBuffer(false) for shader uniform data -- size changes dynamically
    as elements are added/removed, causing shader indexing errors. Use asBuffer(true).
  - Do NOT assume the returned Buffer is a copy -- writes to it modify the stack

Source:
  ScriptingApiObjects.cpp:7286  ScriptUnorderedStack constructor
    -> elementBuffer wraps data.begin() with dynamic size
    -> wholeBf wraps all 128 slots
    -> updateElementBuffer() called after every float-mode mutation
