ScriptAudioWaveform::setDefaultFolder(ScriptObject newDefaultFolder) -> undefined

Thread safety: UNSAFE
Sets the default root directory for the file browser that opens when the user clicks
the waveform to load an audio file. Only affects AudioFile mode.

Required setup:
  const var wf = Content.addAudioWaveform("Waveform1", 0, 0);
  wf.setDefaultFolder(FileSystem.getFolder(FileSystem.Documents));

Anti-patterns:
  - Do NOT pass a string path -- causes script error "newDefaultFolder must be a
    File object". Use FileSystem.getFolder() to obtain a File object.

Source:
  ScriptingApiContent.cpp  ScriptAudioWaveform::setDefaultFolder()
    -> getCachedAudioFile()->getProvider()->setRootDirectory(sf->f)
