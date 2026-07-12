Math (namespace)

Standard math namespace with trigonometry, rounding, clamping, range conversion,
random number generation, and numeric utilities. All methods are pure stateless
functions safe to call from any thread or callback.

Constants:
  math-constants:
    PI = 3.141592653589793       Pi constant
    E = 2.718281828459045        Euler's number
    SQRT2 = 1.4142135623730951   Square root of 2
    SQRT1_2 = 0.7071067811865476 Square root of 0.5 (1/sqrt(2))
    LN2 = 0.6931471805599453     Natural logarithm of 2
    LN10 = 2.302585092994046     Natural logarithm of 10
    LOG2E = 1.4426950408889634   Base-2 logarithm of E
    LOG10E = 0.4342944819032518  Base-10 logarithm of E

Complexity tiers:
  1. Basic arithmetic: range/clamp, abs, min, max, round, floor. Value clamping,
     distance checks, rounding -- used in virtually every script.
  2. Audio-domain math: pow, log10, random, fmod, sin/cos. Perceptual scaling
     curves, step quantization, biased randomization, circular layouts, LFO
     waveform generation.
  3. Range conversion: from0To1, to0To1, skew. Bidirectional mapping between
     normalised (0-1) and real parameter values with skewed distributions.

Practical defaults:
  - Use Math.pow(linearGain, 0.25) to scale linear peak values for visual
    meter display. Fourth-root matches human loudness perception.
  - Use Math.pow(Math.random(), 1.5) for timing humanization. Biases values
    toward zero for natural-sounding small delays.
  - Use value -= Math.fmod(value, stepSize) to snap continuous values to
    discrete steps (zoom levels, grid divisions).
  - Use parseInt(Math.log10(stepSize) * -1) to auto-determine decimal places
    from a step size when displaying slider values.

Common mistakes:
  - Using Math.from0To1 without middlePosition or SkewFactor for frequency
    ranges -- linear mapping is almost never correct for perceptual parameters.
  - Using Math.pow(peak, 0.5) for meter display -- square root does not
    compress enough. Use 0.25 (fourth-root) for proper peak visualisation.
  - Using x + amount * Math.random() for bipolar randomization -- random()
    returns [0, 1). Scale to [-1, 1) first: 2.0 * Math.random() - 1.0.

Example:
  // Math is a global namespace, no instantiation needed
  var freq = Math.from0To1(0.5, {"MinValue": 20.0, "MaxValue": 20000.0, "SkewFactor": 0.3});
  var clamped = Math.range(freq, 20.0, 20000.0);
  var radians = Math.toRadians(180.0); // Math.PI

Methods (41):
  abs         acos        acosh       asin        asinh
  atan        atanh       ceil        clamp       cos
  cosh        exp         floor       fmod        from0To1
  isinf       isnan       log         log10       max
  min         pow         randInt     random      range
  round       sanitize    sign        sin         sinh
  skew        smoothstep  sqr         sqrt        tan
  tanh        to0To1      toDegrees   toRadians   trunc
  wrap
