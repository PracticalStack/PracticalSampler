UnorderedStack::contains(var value) -> Integer

Thread safety: SAFE
Returns true if the stack contains the specified value. Float mode uses exact
float equality via linear scan. Event mode uses the configured compare function.

Dispatch/mechanics:
  Float mode: data.contains((float)value) -- O(n) linear scan
  Event mode: getIndexForEvent(value) != -1 -- iterates events using configured
    compare function (built-in MCF template or custom WeakCallbackHolder)

Anti-patterns:
  - Float comparison uses exact equality -- floating-point precision issues may
    cause false negatives for values that appear equal

Source:
  ScriptingApiObjects.cpp  contains()
    -> float: hise::UnorderedStack::contains() via indexOf()
    -> event: getIndexForEvent() using hcf or compareFunction callback
