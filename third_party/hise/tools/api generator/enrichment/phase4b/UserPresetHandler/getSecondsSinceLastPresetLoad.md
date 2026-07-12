UserPresetHandler::getSecondsSinceLastPresetLoad() -> Double

Thread safety: SAFE
Returns seconds elapsed since the last user preset was loaded. Timer starts
when loadUserPresetInternal begins. Uses Time::getMillisecondCounter() with
millisecond resolution. If no preset has been loaded, returns a large value
(time since application start).
Source:
  MainController.h  timeOfLastPresetLoad
    -> (Time::getMillisecondCounter() - timeOfLastPresetLoad) / 1000.0
