Engine::doubleToString(double value, int digits) -> String

Thread safety: UNSAFE -- String construction (heap allocation)
Returns string representation of a double with the given number of decimal digits.
Source:
  ScriptingApi.cpp  Engine::doubleToString()
    -> String(value, digits)
