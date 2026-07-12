UnorderedStack::setIsEventStack(var isEventStack, var eventCompareFunction) -> undefined

Thread safety: UNSAFE -- custom function path creates a ScriptingMessageHolder
(heap allocation) and configures a WeakCallbackHolder. Built-in compare constant
path is lightweight but the method is a configuration call.
Switches between float mode (default) and event mode. The second parameter
configures the compare function for contains(), remove(), and removeIfEqual().
Callback signature: eventCompareFunction(MessageHolder stackEvent, MessageHolder searchTarget)

Required setup:
  const var es = Engine.createUnorderedStack();
  es.setIsEventStack(true, es.EventId);

Dispatch/mechanics:
  Integer argument: casts to CompareFunctions enum, assigns MCF::equals<> template
  Function argument: sets compareFunctionType = Custom, creates WeakCallbackHolder,
    allocates ScriptingMessageHolder for callback invocations

Anti-patterns:
  - [BUG] EqualData constant (4) is exposed but not implemented in the compare
    template -- always returns false. All contains/remove operations silently fail.
  - [BUG] NoteNumberAndChannel constant (3) checks note number truthiness (non-zero)
    not equality. Note 0 (C-2) never matches; any two non-zero note numbers on
    the same channel match regardless of pitch.
  - Set mode once during initialization. Switching modes does not clear the
    previously active stack's data.

Source:
  ScriptingApiObjects.cpp:7683  setIsEventStack()
    -> integer path: assigns MCF::equals<CompareFunctions> to hcf std::function
    -> function path: WeakCallbackHolder setup + new ScriptingMessageHolder
  ScriptingApiObjects.h:1787  MCF::equals<> template -- switch on CompareFunctions enum
