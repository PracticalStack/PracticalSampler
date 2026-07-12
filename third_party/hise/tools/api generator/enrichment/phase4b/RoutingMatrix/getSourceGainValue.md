RoutingMatrix::getSourceGainValue(Number channelIndex) -> Double

Thread safety: SAFE -- uses ScopedTryReadLock (non-blocking). Returns 0.0 if the lock cannot be acquired.
Returns the current peak level for the given source channel as a linear gain value. Smoothed by internal decay coefficients (upDecayFactor=1.0, downDecayFactor=0.97).
Dispatch/mechanics:
  MatrixData::getGainValue(channelIndex, true) acquires ScopedTryReadLock
    -> reads sourceGainValues[channelIndex]
    -> returns 0.0f if lock unavailable or index out of range
Anti-patterns:
  - Do NOT rely on peak values in exported plugins or when the routing editor is hidden --
    peak metering only runs when anyChannelActive() is true (requires editor reference count).
    All channels return 0.0 unless the routing editor is actively displayed.
Source:
  ScriptingApiObjects.cpp  ScriptRoutingMatrix::getSourceGainValue()
    -> MatrixData::getGainValue(channelIndex, true) in Routing.cpp
    -> ScopedTryReadLock -> sourceGainValues[channelIndex]
