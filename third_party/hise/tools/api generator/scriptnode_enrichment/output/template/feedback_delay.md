---
title: Feedback Delay
description: "A feedback delay loop template with send/receive routing, a delay line, and feedback gain control within fixed 32-sample blocks."
factoryPath: template.feedback_delay
factory: template
polyphonic: false
tags: [template, delay, feedback, routing]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "routing.send", type: companion, reason: "Sends the delayed signal back into the feedback loop" }
  - { id: "routing.receive", type: companion, reason: "Receives the feedback signal and mixes it with the input" }
  - { id: "core.fix_delay", type: companion, reason: "The delay line used internally" }
  - { id: "container.fix32_block", type: companion, reason: "The fixed block size container required for feedback routing" }
commonMistakes:
  - title: "Moving nodes outside the fixed block"
    wrong: "Placing the feedback loop nodes outside the fix32_block container"
    right: "Keep all feedback loop nodes (receive, delay, send) inside the fix32_block container."
    explanation: "The send/receive feedback mechanism requires fixed block sizes. Without the fixed block container, feedback timing becomes unpredictable and host-dependent."
  - title: "Setting Feedback to 1.0 or above"
    wrong: "Setting the Feedback parameter to 1.0 expecting the echoes to sustain indefinitely"
    right: "Keep Feedback below 1.0. At exactly 1.0 the signal never decays; above 1.0 the signal grows and clips."
    explanation: "Each feedback iteration multiplies the signal by the Feedback value. Values at or above unity prevent decay and can cause runaway signal levels."
llmRef: |
  template.feedback_delay

  A composite template that provides a feedback delay loop using send/receive routing within a fixed 32-sample block container. Creates classic delay with exponentially decaying echoes.

  Signal flow:
    input -> receive (mix input + Feedback * delayed) -> delay (DelayTime ms) -> send (back to receive) -> output
    Feedback loop: send --feedback--> receive (one block latency)

  CPU: low, monophonic
    Contains a receive node, a delay line, and a send node. The delay line is the main consumer.

  Parameters:
    No exposed top-level parameters.
    Internal: Feedback (0.0 - 1.0, default 0.0 in template / 0.4 typical), DelayTime (0 - 1000 ms, default 100), FadeTime (0 - 1024 samples, default 512).

  When to use:
    Use as the starting point for any feedback delay effect. Add filtering, saturation, or other processing between the delay and send nodes to shape the feedback character.

  Common mistakes:
    Do not move nodes outside the fix32_block container. Keep Feedback below 1.0 to avoid runaway signal levels.

  See also:
    [companion] routing.send -- sends delayed signal back into the loop
    [companion] routing.receive -- receives and mixes the feedback signal
    [companion] core.fix_delay -- the delay line used internally
    [companion] container.fix32_block -- the fixed block container required for feedback
---

This template provides a ready-made feedback delay loop. It wraps a [receive]($SN.routing.receive$), [delay line]($SN.core.fix_delay$), and [send]($SN.routing.send$) node inside a [fixed 32-sample block container]($SN.container.fix32_block$). The send node routes the delayed output back to the receive node, creating a classic feedback loop with exponentially decaying echoes.

No parameters are exposed at the top level. The internal Feedback, DelayTime, and FadeTime parameters are accessible directly on the child nodes. To expose them, create macro parameter connections from the template's parameter list to the desired child nodes. Users can also insert additional processing between the delay and send nodes to shape the feedback character - for example, a filter for filtered delay or a saturation node for tape-style warmth.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Feedback:
      desc: "Gain multiplier for the feedback signal (internal to fb_out)"
      range: "0.0 - 1.0"
      default: "0.0"
    DelayTime:
      desc: "Delay time in milliseconds (internal to delay node)"
      range: "0 - 1000 ms"
      default: "100"
    FadeTime:
      desc: "Crossfade duration when delay time changes, prevents clicks (internal to delay node)"
      range: "0 - 1024 samples"
      default: "512"
  functions:
    mix feedback:
      desc: "Adds the feedback signal (scaled by Feedback) to the current input"
    delay:
      desc: "Delays the signal by DelayTime milliseconds using a circular buffer"
    send to feedback:
      desc: "Routes the delayed output back to the receive node for the next block"
---

```
// template.feedback_delay - feedback delay loop
// audio in -> audio out (32-sample blocks)

process(input) {
    // receive: mix input with previous feedback
    mixed = mix feedback(input, Feedback)

    // delay the combined signal
    delayed = delay(mixed, DelayTime, FadeTime)

    // send delayed output back to receive
    send to feedback(delayed)

    output = delayed
}
```

::

The feedback path has a minimum latency of 32 samples (one block) due to the send/receive mechanism operating on a per-block basis. At 44100 Hz this is approximately 0.73 ms, small enough to be inaudible as a separate echo.

### Customisation

Users can insert processing nodes between the delay and send nodes (for filtered or shaped feedback) or between the receive and delay nodes (to process the combined input and feedback signal before delaying). The FadeTime parameter controls the crossfade when DelayTime is changed, preventing clicks during delay time modulation.

**See also:** $SN.routing.send$ -- sends delayed signal back into the loop, $SN.routing.receive$ -- receives and mixes the feedback signal, $SN.core.fix_delay$ -- the delay line used internally, $SN.container.fix32_block$ -- the fixed block container required for feedback
