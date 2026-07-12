BeatportManager::setProductId(String productId) -> undefined

Thread safety: UNSAFE -- string construction and console logging in simulation mode; SDK call in production mode.
Sets the Beatport product identifier for this manager instance. In simulation
mode, logs the product ID to the HISE console.

Required setup:
  const bp = Engine.createBeatportManager();

Dispatch/mechanics:
  HISE_INCLUDE_BEATPORT=1: pimpl->setProductId(productId) -- passes to SDK
  HISE_INCLUDE_BEATPORT=0: debugToConsole() -- development stub

Pair with:
  validate -- set product ID before validating
  isBeatportAccess -- set product ID before checking access

Source:
  ScriptExpansion.cpp:3443  BeatportManager::setProductId()
    -> pimpl->setProductId(productId) [SDK mode]
    -> debugToConsole() [simulation mode]
