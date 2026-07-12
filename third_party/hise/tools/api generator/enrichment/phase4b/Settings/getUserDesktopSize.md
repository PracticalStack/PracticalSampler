Settings::getUserDesktopSize() -> Array

Thread safety: UNSAFE -- queries Desktop singleton for display information, constructs an Array
Returns a two-element array [width, height] representing the main display's usable
area in pixels. The usable area excludes the taskbar and other system-reserved regions.

Dispatch/mechanics:
  Desktop::getInstance().getDisplays().getMainDisplay().userArea
    -> [area.getWidth(), area.getHeight()]

Pair with:
  getZoomLevel/setZoomLevel -- compute maximum zoom from display dimensions

Source:
  ScriptingApi.cpp  Settings::getUserDesktopSize()
    -> Desktop::getInstance().getDisplays().getMainDisplay().userArea
