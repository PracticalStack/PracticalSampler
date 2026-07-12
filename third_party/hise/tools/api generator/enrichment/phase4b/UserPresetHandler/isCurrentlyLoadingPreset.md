UserPresetHandler::isCurrentlyLoadingPreset() -> Integer

Thread safety: SAFE
Returns true only when called from the same thread performing the preset load.
Compares the calling thread's handle against currentThreadThatIsLoadingPreset
set at the start of loadUserPresetInternal. Returns false if no load is in
progress or if called from a different thread.
Dispatch/mechanics:
  LockHelpers::getCurrentThreadHandleOrMessageManager()
    == currentThreadThatIsLoadingPreset
Source:
  MainController.h:941  isCurrentlyInsidePresetLoad()
    -> thread handle comparison (no locks)
