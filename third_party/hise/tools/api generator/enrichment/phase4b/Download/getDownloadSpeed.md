Download::getDownloadSpeed() -> Integer

Thread safety: SAFE -- reads atomic bool and two int64 members. No allocations, no locks.
Returns the current download speed in bytes per second. Uses a sliding one-second window;
returns max of current and previous window to avoid zero readings at boundaries.
Returns 0 when the download is not actively running.

Dispatch/mechanics:
  Reads isRunning_ flag; if false returns 0
  Returns jmax(bytesInLastSecond, bytesInCurrentSecond)
  Speed counters updated in progress() callback on WebThread

Source:
  ScriptingApiObjects.cpp:~1320  ScriptDownloadObject::getDownloadSpeed()
    -> isRunning() ? jmax((int)bytesInLastSecond, (int)bytesInCurrentSecond) : 0
