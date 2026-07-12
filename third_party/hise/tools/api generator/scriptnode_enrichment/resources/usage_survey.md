# Scriptnode Node Usage Survey

> Auto-generated from DspNetwork XML files across all surveyed networks.
> Used by Step 4 agents for "when to use" guidance and identifying common node combinations.

## Survey Scope

- XML files scanned: 84
- Networks containing nodes: 80
- Total node instances: 1612
- Unique standard nodes used: 112 of 194 (57.7%)
- Unused standard nodes: 82
- Project-specific (`project.*`) nodes also observed: 21 unique types, 30 instances

---

## 1. Node Usage Frequency

Sorted by instance count (descending). All 112 used nodes shown.

| Rank | Node | Instances |
|-----:|------|----------:|
| 1 | container.chain | 351 |
| 2 | core.gain | 113 |
| 3 | math.mul | 81 |
| 4 | container.split | 58 |
| 5 | control.cable_expr | 57 |
| 6 | filters.svf_eq | 42 |
| 7 | control.xfader | 40 |
| 8 | container.multi | 36 |
| 9 | core.oscillator | 34 |
| 10 | filters.one_pole | 29 |
| 11 | math.clear | 29 |
| 12 | core.peak | 28 |
| 13 | container.soft_bypass | 27 |
| 14 | control.converter | 27 |
| 15 | control.pma_unscaled | 22 |
| 16 | filters.biquad | 21 |
| 17 | routing.receive | 21 |
| 18 | routing.send | 21 |
| 19 | math.add | 20 |
| 20 | container.frame2_block | 19 |
| 21 | jdsp.jpanner | 19 |
| 22 | control.normaliser | 18 |
| 23 | core.fix_delay | 18 |
| 24 | math.tanh | 17 |
| 25 | container.midichain | 16 |
| 26 | control.blend | 16 |
| 27 | jdsp.jlinkwitzriley | 14 |
| 28 | fx.phase_delay | 13 |
| 29 | math.sig2mod | 13 |
| 30 | routing.global_cable | 13 |
| 31 | routing.ms_decode | 13 |
| 32 | routing.ms_encode | 13 |
| 33 | control.tempo_sync | 12 |
| 34 | filters.svf | 12 |
| 35 | control.pma | 11 |
| 36 | jdsp.jdelay | 11 |
| 37 | container.branch | 10 |
| 38 | container.modchain | 10 |
| 39 | core.snex_node | 10 |
| 40 | dynamics.envelope_follower | 10 |
| 41 | math.clip | 10 |
| 42 | control.clone_cable | 9 |
| 43 | control.midi | 9 |
| 44 | filters.linkwitzriley | 9 |
| 45 | math.mod_inv | 9 |
| 46 | control.smoothed_parameter | 8 |
| 47 | filters.convolution | 8 |
| 48 | jdsp.jdelay_cubic | 8 |
| 49 | routing.matrix | 8 |
| 50 | control.bang | 7 |
| 51 | dynamics.comp | 7 |
| 52 | fx.bitcrush | 7 |
| 53 | container.no_midi | 6 |
| 54 | fx.reverb | 6 |
| 55 | math.table | 6 |
| 56 | control.smoothed_parameter_unscaled | 5 |
| 57 | core.ramp | 5 |
| 58 | envelope.ahdsr | 5 |
| 59 | envelope.simple_ar | 5 |
| 60 | fx.sampleandhold | 4 |
| 61 | container.clone | 3 |
| 62 | container.fix128_block | 3 |
| 63 | container.fix32_block | 3 |
| 64 | control.change | 3 |
| 65 | control.clone_pack | 3 |
| 66 | control.input_toggle | 3 |
| 67 | control.minmax | 3 |
| 68 | control.random | 3 |
| 69 | control.voice_bang | 3 |
| 70 | core.mono2stereo | 3 |
| 71 | math.abs | 3 |
| 72 | math.expr | 3 |
| 73 | math.fill1 | 3 |
| 74 | math.sub | 3 |
| 75 | routing.public_mod | 3 |
| 76 | analyse.fft | 2 |
| 77 | container.fix16_block | 2 |
| 78 | container.fix64_block | 2 |
| 79 | container.fix8_block | 2 |
| 80 | container.offline | 2 |
| 81 | container.oversample4x | 2 |
| 82 | control.bipolar | 2 |
| 83 | control.cable_table | 2 |
| 84 | control.logic_op | 2 |
| 85 | control.midi_cc | 2 |
| 86 | control.pack_resizer | 2 |
| 87 | control.resetter | 2 |
| 88 | control.unscaler | 2 |
| 89 | core.clock_ramp | 2 |
| 90 | core.extra_mod | 2 |
| 91 | core.file_player | 2 |
| 92 | core.granulator | 2 |
| 93 | core.phasor | 2 |
| 94 | core.pitch_mod | 2 |
| 95 | core.smoother | 2 |
| 96 | dynamics.gate | 2 |
| 97 | envelope.flex_ahdsr | 2 |
| 98 | math.div | 2 |
| 99 | math.fmod | 2 |
| 100 | math.pi | 2 |
| 101 | math.rect | 2 |
| 102 | math.sin | 2 |
| 103 | routing.selector | 2 |
| 104 | analyse.oscilloscope | 1 |
| 105 | core.faust | 1 |
| 106 | core.global_mod | 1 |
| 107 | core.matrix_mod | 1 |
| 108 | core.recorder | 1 |
| 109 | core.snex_shaper | 1 |
| 110 | envelope.voice_manager | 1 |
| 111 | math.pack | 1 |
| 112 | math.neural | 1 |

---

## 2. Per-Factory Usage Summary

| Factory | Instances | Nodes Used | Nodes Available | Coverage |
|---------|----------:|-----------:|----------------:|---------:|
| analyse | 2 | 2 | 4 | 50.0% |
| container | 549 | 17 | 28 | 60.7% |
| control | 262 | 25 | 47 | 53.2% |
| core | 231 | 19 | 25 | 76.0% |
| dynamics | 19 | 3 | 5 | 60.0% |
| envelope | 16 | 4 | 7 | 57.1% |
| filters | 121 | 6 | 10 | 60.0% |
| fx | 30 | 4 | 6 | 66.7% |
| jdsp | 53 | 5 | 7 | 71.4% |
| math | 204 | 19 | 26 | 73.1% |
| routing | 95 | 8 | 14 | 57.1% |
| template | 0 | 0 | 15 | 0.0% |
| **Total** | **1582** | **112** | **194** | **57.7%** |

### Observations

- **container** dominates instance counts (549 total), with `container.chain` alone at 351 - the fundamental building block of every network.
- **core** has the highest coverage (76%) - most core DSP primitives see real use.
- **template** has 0% coverage - template nodes are meta-containers expanded at compile time and do not appear as raw FactoryPath entries in saved XML. Their constituent nodes appear under their actual factory paths instead.
- **control** is the largest factory (47 nodes) but only 53.2% coverage - many specialized cable/pack writers are rarely needed.
- **math** is heavily used (204 instances) with good coverage (73.1%), confirming its role as the signal-shaping workhorse.

### Project-Specific Nodes

21 unique `project.*` nodes were observed (30 instances total). These are custom networks referenced as reusable sub-graphs within a project. Common patterns include custom saturators, compressors, filters, and LFOs wrapped as project-level nodes.

---

## 3. Top 20 Node Co-Occurrence Pairs

Pairs of nodes that appear together in the same network, ranked by number of networks containing both.

| Rank | Node A | Node B | Networks |
|-----:|--------|--------|:--------:|
| 1 | container.chain | math.mul | 38 |
| 2 | container.chain | core.gain | 37 |
| 3 | container.chain | container.split | 34 |
| 4 | container.chain | control.xfader | 30 |
| 5 | container.split | core.gain | 27 |
| 6 | container.chain | container.multi | 25 |
| 7 | container.split | math.mul | 24 |
| 8 | core.gain | math.mul | 22 |
| 9 | container.split | control.xfader | 21 |
| 10 | container.chain | core.peak | 21 |
| 11 | control.xfader | core.gain | 21 |
| 12 | container.chain | filters.one_pole | 19 |
| 13 | control.xfader | math.mul | 19 |
| 14 | container.chain | container.frame2_block | 18 |
| 15 | container.chain | control.cable_expr | 17 |
| 16 | container.chain | math.clear | 17 |
| 17 | container.multi | core.gain | 17 |
| 18 | core.peak | math.mul | 17 |
| 19 | container.split | core.peak | 16 |
| 20 | container.split | filters.one_pole | 15 |

### Key Patterns

- **container.chain + core.gain + math.mul** is the most common triad - nearly every network uses gain staging with multiplication for level control.
- **container.split + control.xfader** appears in 21 networks, indicating a dominant pattern for parallel processing with crossfade mix control (dry/wet, parallel bands).
- **core.peak + math.mul** (17 networks) shows frequent use of peak metering combined with signal scaling, often for envelope-following or level-dependent processing.
- **container.frame2_block** co-occurs heavily with container.chain (18 networks), confirming its role as the standard frame-processing wrapper for per-sample DSP.
- **filters.one_pole** pairs frequently with both container.chain (19) and container.split (15), used as a smoothing/envelope filter across many effect types.
- **routing.ms_encode + routing.ms_decode** always appear together (13 networks each), forming the standard mid/side processing bracket.

---

## 4. Notable Parameter Configurations

Non-default parameter values observed for frequently used nodes.

### core.gain (113 instances)

Most instances use default parameters. One instance sets:
- `Smoothing`: 16.0ms (non-default; default is likely 0 or 20ms depending on context)

### math.mul (81 instances, 65 with parameters)

Common non-default `Value` settings:
- `4.0` (x2), `0.25` (x2), `2.0` (x2)
- Used for octave scaling (x4, x0.25) and doubling/halving signals

### core.oscillator (34 instances)

- `Mode`: 1.0 (x19), 4.0 (x2), 2.0 (x1) - Mode 1 (Saw) dominates over Mode 0 (Sine default)
- Saw is the most common oscillator waveform in surveyed networks

### container.chain (351 instances, 311 with parameters)

Parameters on container.chain reflect the parameters of the network itself (exposed to the host). Notable patterns:
- `Mode`: 1.0 (x18), 2.0 (x4) - frequently switched from default mode
- `Limit`: 20.0 (x4), 50.0 (x3), 10.0 (x2) - delay/range limits commonly customized
- `Smoothing`: 0.0 (x5) - explicitly disabled in several networks
- `Multithread`: 1.0 (x3) - multithreading enabled for CPU-heavy networks
- AHDSR-style parameters appear frequently (Attack, Decay, Sustain, Release with various curves)

### filters.one_pole (29 instances)

- `Smoothing`: 0.0 (x4) - parameter smoothing explicitly disabled when used for audio-rate filtering rather than control smoothing

### control.blend (16 instances)

Used for interpolating between two modulation values. Typically uses default Alpha=0 configuration with modulation connections driving the blend.

### dynamics.envelope_follower (10 instances)

Frequently paired with math.mul and core.peak for sidechain-style processing and level-dependent effects.

---

## 5. Unused Nodes (82 of 194)

Nodes present in the canonical node list but absent from all surveyed networks.

### analyse (2 unused of 4)

- `analyse.goniometer`
- `analyse.specs`

### container (11 unused of 28)

- `container.dynamic_blocksize`
- `container.fix256_block`
- `container.fix_blockx`
- `container.frame1_block`
- `container.framex_block`
- `container.oversample` (generic variant)
- `container.oversample16x`
- `container.oversample2x`
- `container.oversample8x`
- `container.repitch`
- `container.sidechain`

### control (22 unused of 47)

- `control.branch_cable`
- `control.cable_pack`
- `control.clone_forward`
- `control.compare`
- `control.delay_cable`
- `control.file_analyser`
- `control.intensity`
- `control.locked_mod`
- `control.locked_mod_unscaled`
- `control.pack2_writer`
- `control.pack3_writer`
- `control.pack4_writer`
- `control.pack5_writer`
- `control.pack6_writer`
- `control.pack7_writer`
- `control.pack8_writer`
- `control.ppq`
- `control.sliderbank`
- `control.timer`
- `control.transport`
- `control.xy`

### core (6 unused of 25)

- `core.fm`
- `core.peak_unscaled`
- `core.phasor_fm`
- `core.snex_osc`
- `core.stretch_player`
- `core.table`

### dynamics (2 unused of 5)

- `dynamics.limiter`
- `dynamics.updown_comp`

### envelope (3 unused of 7)

- `envelope.extra_mod_gate`
- `envelope.global_mod_gate`
- `envelope.silent_killer`

### filters (4 unused of 10)

- `filters.allpass`
- `filters.ladder`
- `filters.moog`
- `filters.ring_mod`

### fx (2 unused of 6)

- `fx.haas`
- `fx.pitch_shift`

### jdsp (2 unused of 7)

- `jdsp.jchorus`
- `jdsp.jdelay_thiran`

### math (7 unused of 26)

- `math.intensity`
- `math.inv`
- `math.map`
- `math.mod2sig`
- `math.pow`
- `math.sqrt`
- `math.square`

### routing (6 unused of 14)

- `routing.event_data_reader`
- `routing.event_data_writer`
- `routing.global_receive`
- `routing.global_send`
- `routing.local_cable`
- `routing.local_cable_unscaled`

### template (15 unused of 15 - entire factory)

- `template.bipolar_mod`
- `template.dry_wet`
- `template.feedback_delay`
- `template.freq_split2`
- `template.freq_split3`
- `template.freq_split4`
- `template.freq_split5`
- `template.mid_side`
- `template.softbypass_switch2`
- `template.softbypass_switch3`
- `template.softbypass_switch4`
- `template.softbypass_switch5`
- `template.softbypass_switch6`
- `template.softbypass_switch7`
- `template.softbypass_switch8`

### Notes on Unused Nodes

- **template factory (15 nodes):** These are compile-time meta-containers that expand into standard nodes. They will never appear as raw FactoryPath entries in saved XML - their absence is expected and does not indicate lack of use.
- **control.pack[2-8]_writer (7 nodes):** Specialized fixed-size pack writers. Projects may prefer script-driven approaches or use different pack sizes.
- **container.oversample variants:** Only `container.oversample4x` was observed (2 instances). The generic and other fixed-rate variants were unused in this sample.
- **routing.global_send/global_receive:** These may be superseded by `routing.global_cable` (13 instances) which provides a more flexible global routing mechanism.
- **routing.local_cable/local_cable_unscaled:** Newer routing primitives that may not yet be widely adopted.
- **filters.ladder, filters.moog:** Classic analog-modeled filters - absence may reflect preference for the more flexible `filters.svf` and `filters.svf_eq` in practice.
- **dynamics.limiter:** Surprisingly absent - networks may implement limiting via `dynamics.comp` with high ratios, or use HISE's built-in module-level limiter instead.
- **core.fm, core.phasor_fm:** FM synthesis primitives - absence suggests FM is typically built from `core.oscillator` + modulation routing rather than dedicated FM nodes.
