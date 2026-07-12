Table::getCurrentlyDisplayedIndex() -> Double

Thread safety: SAFE
Returns the last ruler position sent to the table's display updater, as a normalized
value between 0.0 and 1.0. Updated whenever getTableValueNormalised() is called or
when a module queries the table during audio processing.

Source:
  ScriptingApiObjects.cpp:2156  getCurrentlyDisplayedIndex()
    -> getCurrentDisplayIndexBase() -> complexObject->getUpdater().getLastDisplayValue()
