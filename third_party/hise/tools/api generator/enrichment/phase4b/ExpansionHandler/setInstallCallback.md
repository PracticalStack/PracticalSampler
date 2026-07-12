ExpansionHandler::setInstallCallback(var installationCallback) -> undefined

Thread safety: UNSAFE -- creates a WeakCallbackHolder on the heap, increments ref
count, registers as source.
Sets a callback for tracking expansion installation progress. Fires at start
(Status 0), periodically during install at 300ms intervals (Status 1), and on
completion (Status 2). Must be set before calling installExpansionFromPackage().
Callback signature: f(Object state)
  state.Status: int (-1=not started, 0=started, 1=in progress, 2=complete)
  state.Progress: double (sample preload progress 0.0-1.0)
  state.TotalProgress: double (overall progress 0.0-1.0)
  state.SourceFile: File (.hr package being installed)
  state.TargetFolder: File (expansion root directory)
  state.SampleFolder: File (sample destination directory)
  state.Expansion: Expansion or undefined (only valid at Status 2)
Pair with:
  installExpansionFromPackage -- must set this callback before installing
Source:
  ScriptExpansion.cpp  setInstallCallback()
    -> creates InstallState (Timer + ExpansionHandler::Listener)
    -> InstallState timer fires every 300ms
    -> InstallState::getObject() builds status JSON per timer tick
