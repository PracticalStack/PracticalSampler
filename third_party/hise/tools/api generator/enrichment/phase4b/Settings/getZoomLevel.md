Settings::getZoomLevel() -> Double

Thread safety: SAFE
Returns the current global UI scale factor. 1.0 means 100% zoom (no scaling).

Pair with:
  setZoomLevel -- change the zoom level

Source:
  ScriptingApi.cpp  Settings::getZoomLevel()
    -> gm->getGlobalScaleFactor()
