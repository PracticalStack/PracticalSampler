# template.feedback_delay - Composite Exploration

**Root container:** `container.fix32_block`
**Classification:** container (composite template)

## Signal Path

The root is a `container.fix32_block`, which processes audio in fixed 32-sample chunks. This fixed block size is required for the routing.send/receive feedback mechanism to work correctly (the send/receive pair operates on a per-block basis, so fixed block size ensures consistent feedback delay).

Internal serial chain (within 32-sample blocks):

1. **fb_out** (`routing.receive`) -- receives the feedback signal sent by fb_in. The Feedback parameter (default 0.4 = 40%) controls the gain of the received signal, which is mixed (added) with the incoming audio. At Feedback=0, no feedback is applied and the node passes audio through unchanged.
2. **delay** (`core.fix_delay`) -- delays the signal by DelayTime milliseconds (default 100ms). FadeTime (default 512 samples) controls the crossfade duration when the delay time is changed, preventing clicks.
3. **fb_in** (`routing.send`) -- sends the delayed output back to fb_out via the Connection property set to "fb_out". This completes the feedback loop.

The feedback loop creates a classic delay effect: input + feedback*delayed_signal is passed through the delay, and the delayed result is fed back. Each iteration of the feedback loop attenuates by the Feedback amount, creating exponentially decaying echoes.

## Gap Answers

### feedback-loop-mechanics

The routing.receive node (fb_out) mixes the feedback signal with the incoming audio additively. The Feedback parameter acts as a gain multiplier on the received signal before it is added to the input. So the signal at the output of fb_out is: `input + Feedback * delayed_signal`. This additive mixing means the user hears both the original signal and the delayed echoes. The routing.send/receive pair works on a per-block basis -- the send writes its input to an internal buffer, and the receive reads from that buffer on the next processing cycle, creating a minimum latency of one block (32 samples at the fixed block size).

### fix32-block-requirement

The container.fix32_block is required because routing.send/receive feedback operates on a per-block basis. The send node writes to a shared buffer, and the receive node reads from it on the next block. Without fixed block sizes, the feedback latency would vary with the host buffer size, making the delay timing unpredictable. The 32-sample block size sets the minimum feedback loop latency to 32 samples (~0.73ms at 44100 Hz), which is small enough to be inaudible as a separate echo. Users could change the root to a different fixed block size (fix8_block, fix64_block) but should not use a non-fixed container as the feedback timing would become host-dependent.

### no-exposed-parameters

This template intentionally exposes no top-level parameters. The internal child node parameters (Feedback on fb_out, DelayTime and FadeTime on delay) are accessible directly on the child nodes. The expected workflow is for users to either: (1) access the child nodes directly and adjust their parameters in the scriptnode IDE, or (2) manually create macro parameter connections from the template's parameter list to the desired child node parameters. This gives users full flexibility in which parameters to expose.

### description-accuracy

The base description "Split the audio signal into fixed length chunks of 32 samples" is inherited from container.fix32_block and describes only the root container's block processing, not the feedback delay functionality. A more accurate description: "A feedback delay loop template with send/receive routing, delay line, and feedback gain control within 32-sample fixed blocks."

### user-processing-insertion

Users can insert additional processing nodes between fb_out (receive) and fb_in (send) to shape the feedback character. The typical insertion point is between the delay node and fb_in (send) -- for example, inserting a filter to create a filtered feedback delay, or a saturation node for tape-style delay. Users can also insert nodes before the delay (between fb_out and delay) to process the combined input+feedback signal before delaying. Both approaches are valid and common in feedback delay design.

## Internal Parameters (not exposed)

- **fb_out.Feedback** (0..1, default 0.0, current value 0.4): Gain multiplier for the feedback signal. NormalizedPercentage display. At 1.0, feedback is at unity (infinite sustain, potentially unstable).
- **delay.DelayTime** (0..1000ms, default 100ms, skew 0.301): Delay time. Logarithmic skew for natural frequency-like control feel.
- **delay.FadeTime** (0..1024 samples, default 512): Crossfade time when delay time changes. Prevents clicks during delay time modulation.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []

The template contains a receive node, a delay line, and a send node. The delay line is the main CPU consumer, but core.fix_delay is efficient (simple circular buffer). The 32-sample block processing adds minimal overhead from the ChunkableProcessData splitting.

## Notes

- The feedback path has an inherent minimum latency of 32 samples (one block) due to the send/receive mechanism operating on a per-block basis.
- At Feedback values approaching 1.0, the delay will produce very long sustaining echoes. At exactly 1.0, the signal never decays (infinite feedback). Values above 1.0 would cause the signal to grow and eventually clip.
- The fb_in send node's Connection property is set to "fb_out", establishing the named routing link.
