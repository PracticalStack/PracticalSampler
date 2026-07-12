UnorderedStack (object)
Obtain via: Engine.createUnorderedStack()

Lock-free fixed-capacity container (128 elements) for floats or HISE events
with set-like insert/remove semantics. Audio-thread safe. Operates in float
mode (default) or event mode with configurable compare functions.

Constants:
  CompareFunction:
    BitwiseEqual = 0           Compare all event fields for exact equality
    EventId = 1                Compare event IDs only (matches note-on/off pairs)
    NoteNumberAndVelocity = 2  Match note-on events with same note number and velocity
    NoteNumberAndChannel = 3   Match events with same note number and channel
    EqualData = 4              Not implemented -- always returns false

Complexity tiers:
  1. Float note tracker: insert, remove, contains, clear, size. Simple set
     operations on note numbers for keyboard highlighting and chord detection.
  2. Buffer-backed visualization: + asBuffer, copyTo. Live buffer views for
     iterating active elements or feeding fixed-size shader uniform data.
  3. Event-mode MIDI lifecycle: + setIsEventStack, removeIfEqual, storeEvent,
     removeElement. Full note-on/off lifecycle tracking with original event
     metadata preservation.

Practical defaults:
  - Use float mode (the default) for note number tracking. Event mode is only
    needed when you must recover the original note-on event's full metadata at
    note-off time.
  - Use EventId as the compare function for event-mode stacks that track
    note-on/off pairs. HISE assigns matching event IDs to paired note events.
  - Create a single MessageHolder with Engine.createMessageHolder() and reuse
    it across callbacks. Message.store(holder) overwrites the holder's content
    each time -- no need to create a new holder per event.
  - Use asBuffer(true) (all 128 slots) when feeding shader uniforms -- shaders
    expect fixed-size arrays. Use asBuffer(false) for iterating only occupied
    elements in script logic.

Common mistakes:
  - Inserting a MessageHolder without calling setIsEventStack first -- float
    mode is the default, insert silently returns false for MessageHolder values.
  - Using EqualData (constant 4) as compare function -- not implemented,
    contains() always returns false, remove()/removeIfEqual() never match.
  - Using NoteNumberAndChannel (constant 3) with note C-2 (number 0) -- bug
    in implementation checks note number truthiness, not equality. Note 0
    never matches.
  - Creating a new MessageHolder in every note-on callback -- heap allocation.
    Reuse a single holder with Message.store(holder).
  - Iterating asBuffer(false) with index-based for loop while removing --
    removal swaps in the last element, skipping entries. Use a while-drain
    loop with storeEvent(0, holder) + removeElement(0) instead.
  - copyTo with a Buffer target of exactly the same size as the stack --
    requires strictly larger buffer due to off-by-one in size check.

Example:
  // Float mode (default)
  const us = Engine.createUnorderedStack();
  us.insert(60.0);
  us.insert(64.0);
  us.insert(67.0);
  Console.print(us.size()); // 3
  Console.print(us.contains(64.0)); // true

  // Event mode
  const es = Engine.createUnorderedStack();
  es.setIsEventStack(true, es.EventId);

Methods (12):
  asBuffer        clear           contains
  copyTo          insert          isEmpty
  remove          removeElement   removeIfEqual
  setIsEventStack size            storeEvent
