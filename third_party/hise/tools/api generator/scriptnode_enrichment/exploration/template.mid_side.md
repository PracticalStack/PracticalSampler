# template.mid_side - Composite Exploration

**Root container:** `container.chain`
**Classification:** container (composite template)

## Signal Path

The root is a `container.chain` (serial processing). The template implements a mid/side processing framework in three stages:

1. **decoder** (`routing.ms_decode`) -- converts stereo L/R signal to Mid/Side encoding. Standard matrix: Mid = (L+R), Side = (L-R). The ms_decode node operates on a 2-channel signal, transforming channel 0 to mid and channel 1 to side.

2. **ms_splitter** (`container.multi`) -- splits the 2-channel M/S signal into two mono chains using container.multi's channel-splitting behaviour. Channel 0 (mid) goes to mid_chain, channel 1 (side) goes to side_chain.
   - **mid_chain** (`container.chain`) -- processes the mono mid signal. Contains **mid_gain** (`math.mul`, Value=1.0 = passthrough) as a placeholder.
   - **side_chain** (`container.chain`) -- processes the mono side signal. Contains **side_gain** (`math.mul`, Value=1.0 = passthrough) as a placeholder.

3. **encoder** (`routing.ms_encode`) -- converts Mid/Side signal back to stereo L/R. Inverse matrix: L = Mid+Side, R = Mid-Side.

The round-trip (decode -> encode) is unity gain when no processing is applied in the mid/side chains. The math.mul placeholders at Value=1.0 pass audio through unchanged.

## Gap Answers

### ms-decode-encode-algorithm

The routing.ms_decode and routing.ms_encode use the standard M/S matrix. Decode: Mid = L+R, Side = L-R (without the 0.5 scaling factor -- the scaling is split between encode and decode or handled as a pair). Encode: L = (Mid+Side)/2, R = (Mid-Side)/2. The exact scaling ensures the round-trip is unity: L_out = ((L+R)+(L-R))/2 = L, R_out = ((L+R)-(L-R))/2 = R. The implementation uses simple addition and subtraction on the two channel samples per frame.

### multi-container-channel-split

Yes, the container.multi assigns channels sequentially to its children. With 2 children (mid_chain and side_chain), each getting 1 channel: mid_chain processes channel 0 (mid) and side_chain processes channel 1 (side). Both chains are mono (1 channel each). The total channel count of the multi container is the sum of its children's channels (1+1=2), matching the 2-channel M/S signal from the decoder.

### no-exposed-parameters

This template exposes no top-level parameters. The internal mid_gain and side_gain (math.mul) nodes have Value parameters at 1.0 (passthrough). The intended workflow is for users to: (1) add their own processing nodes in the mid_chain and/or side_chain to apply different effects to mid vs side content, and/or (2) adjust mid_gain/side_gain values to control mid/side balance, and/or (3) expose the internal gain parameters as macro parameters if they want top-level control.

### description-accuracy

The base description "A container for serial processing of nodes" is inherited from container.chain and completely fails to describe the template's M/S processing purpose. A more accurate description: "A mid/side processing template that decodes stereo to M/S, processes mid and side independently, then re-encodes to stereo."

### stereo-requirement

This template requires exactly 2-channel (stereo) input. The routing.ms_decode expects 2 channels (L/R) and the container.multi splits into 2 mono channels. In a mono context, the side signal would be zero (L-R=0 when L=R), making the side chain ineffective. In a multichannel context (>2 channels), extra channels would not be processed by the M/S framework and behaviour would be undefined.

## Internal Parameters (not exposed)

- **mid_gain.Value** (0..1, default 1.0): Gain multiplier for the mid signal. At 0, the mid (centre) content is removed.
- **side_gain.Value** (0..1, default 1.0): Gain multiplier for the side signal. At 0, the side (stereo width) content is removed, producing a mono output.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

The template adds only matrix encode/decode (2 additions per sample) and two gain multiplies. Actual CPU cost depends on what the user places in the mid/side chains.

## Notes

- Available image: ms_after.png
- This is the only template with a non-stub existing doc (BRIEF tier, 10 lines).
- Reducing side_gain to 0 narrows stereo width to mono. Increasing mid_gain relative to side_gain emphasizes the centre content. This is a standard M/S technique for stereo width control.
- Users commonly add EQ, compression, or saturation independently to mid and side chains for mastering applications.
