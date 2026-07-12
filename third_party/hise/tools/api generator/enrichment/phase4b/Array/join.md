Array::join(String separator) -> String

Thread safety: UNSAFE -- always allocates strings. Guarded with
WARN_IF_AUDIO_THREAD for string creation.
Converts all elements to strings and concatenates them with the specified
separator between each element.

Anti-patterns:
  - Do NOT call from audio callbacks -- always triggers the audio thread
    safety warning due to string allocation.

Source:
  JavascriptEngineObjects.cpp  ArrayClass::join()
    -> WARN_IF_AUDIO_THREAD(true, IllegalAudioThreadOps::StringCreation)
    -> iterates elements, builds String with separator
