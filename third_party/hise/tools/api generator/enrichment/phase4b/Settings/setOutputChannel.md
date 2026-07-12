Settings::setOutputChannel(Integer index) -> undefined

Thread safety: UNSAFE -- calls CustomSettingsWindow::flipEnablement() which modifies device manager channel config
Selects an output channel pair by stereo pair index. The index corresponds to a
stereo pair from getAvailableOutputChannels(), not an individual channel number.
Primarily useful in standalone builds.

Anti-patterns:
  - Do NOT pass individual channel numbers -- the index is a stereo pair index
    (0 = first pair, 1 = second pair). Passing a channel number produces
    unexpected results.

Pair with:
  getAvailableOutputChannels -- list available stereo pairs
  getCurrentOutputChannel -- read the active pair index

Source:
  ScriptingApi.cpp  Settings::setOutputChannel()
    -> CustomSettingsWindow::flipEnablement(driver->deviceManager, index)
