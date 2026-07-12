# routing.matrix - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/RoutingNodes.h:1239`
**Base class:** `HiseDspBase` (implicit, via SN_GET_SELF_AS_OBJECT)
**Classification:** audio_processor

## Signal Path

The matrix node applies an arbitrary channel routing defined by its `MatrixType` template parameter. Processing occurs frame-by-frame via `processMatrixFrame()` (line 1293-1322).

The algorithm:
1. Copy all channel data into a temporary local buffer `chData[]` (line 1301-1302)
2. Zero the output channels: `data[i] = 0.0f` (line 1303)
3. For each source channel `i`, look up its destination via `m.getChannel(i)` (line 1309)
4. If destination is not -1, add the source to the destination: `data[index] += chData[i]` (line 1311)
5. If the matrix type supports send channels (`hasSendChannels()`), also route to the send destination via `m.getSendChannel(i)` (lines 1315-1319)

This means multiple source channels can be summed into one destination (additive mixing). A channel mapped to -1 is effectively muted.

For 2-channel fixed matrices, the code has optimised special-case paths (via `matrix_helpers::applySpecialType`) that avoid frame-by-frame processing. The special types are: AddRightToLeft, AddLeftToRight, SwapChannels, LeftOnly, RightOnly. These use `FloatVectorOperations` for block-level processing (lines 1098-1129).

## Gap Answers

### matrix-processing-logic: How does the routing matrix apply the channel mapping?

The matrix copies all input channels to a temporary buffer, zeros the outputs, then for each source channel adds it to the mapped destination channel. Multiple sources can be summed into one destination. The mapping is a per-channel integer index (-1 = muted, 0..N = destination channel index). Send channels provide a secondary destination for each source, enabling parallel routing without signal loss.

### embedded-data-format: What is the format of the EmbeddedData string?

The MatrixType template parameter handles data format via its `initialise(ObjectWithValueTree*)` method. The C++ code in this header does not define the serialisation format -- it is handled by the MatrixType implementation (typically a dynamic matrix class in the HISE runtime, not defined in RoutingNodes.h). For `static_matrix` types (line 1338), the routing is compile-time constant via `SubType::channels[]` and `SubType::sendChannels[]` arrays, requiring no serialised data.

### channel-count-flexibility: Does the matrix adapt to the channel count of the signal path?

The matrix operates on however many channels are in the ProcessData. The `processMatrixFrame()` loop iterates `data.size()` channels (line 1301, 1307). `MatrixType::getNumChannels()` defines the maximum channel count, and the temporary buffer is allocated to that size. For `isFixedChannelMatrix()` types, the channel count is compile-time fixed. For dynamic matrix types, `forwardToFrame16()` dispatches up to 16 channels at runtime.

### processing-overhead: Does the matrix copy audio buffers or use pointer swapping?

The matrix always copies data through a temporary buffer -- there is no pointer swapping optimisation. However, for 2-channel fixed matrices with common routing patterns, the special-type optimisation uses block-level `FloatVectorOperations` (copy, add, clear) which is significantly faster than frame-by-frame processing. For identity routing (channel 0->0, 1->1), `getSpecialType()` returns `NoSpecialType` so it falls through to frame processing. The `handleDisplayValues()` calls (lines 1263, 1277) add minor overhead for metering.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: [{"parameter": "channel count", "impact": "linear", "note": "Cost scales linearly with number of channels; special-case 2-channel paths are faster"}]

## Notes

The `static_matrix<N, SubType, HasSendChannels>` class (line 1338) provides compile-time fixed routing for C++ export. It reads channel mappings from `SubType::channels[]` and `SubType::sendChannels[]` static arrays. The `createDisplayValues()` returns false for static matrices (no metering needed). The matrix node has no parameters (`createParameters` is empty, line 1326-1329).
