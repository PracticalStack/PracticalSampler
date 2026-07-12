Returns a JSON object describing the current Lottie animation state with properties `active`, `currentFrame`, `numFrames`, and `frameRate`. These properties update live when you load a new animation or change the frame, so you only need to call this method once and then read the properties directly.

A typical pattern is to use `frameRate` to set the timer interval: `pnl.startTimer(1000.0 / data.frameRate)`.
