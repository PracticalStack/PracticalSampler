#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/DspChorus.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
}

int main()
{
    using namespace drs::engine;
    try
    {
        const auto* descriptor = findCuratedDspEffect("drs.chorus", 1);
        require(descriptor != nullptr && descriptor->parameters.size() == 5 && descriptor->cost.stateBytes == DspChorusState::maximumStateBytes,
                "Chorus v1 requires a fixed three-voice catalog contract.");
        DspChorusState state;
        require(state.prepare(48000.0), "Chorus must allocate all fixed voices during preparation.");
        const auto firstAllocation = state.voices[0].left.data();
        state.prepare(96000.0);
        require(state.voices[0].left.data() == firstAllocation, "Chorus sample-rate preparation must reuse fixed voice allocations.");

        std::array<float, 2048> reference {}, singleBlock {}, splitBlocks {};
        reference.front() = singleBlock.front() = splitBlocks.front() = 1.0f;
        DspChorusParameters parameters; parameters.rateHz = 1.7; parameters.depthMs = 8.0; parameters.baseDelayMs = 12.0; parameters.mix = 1.0;
        DspChorusState oneBlock, split; oneBlock.prepare(48000.0); split.prepare(48000.0);
        float* oneChannel[] { singleBlock.data() };
        processDspChorusRamp({ oneChannel, 1, static_cast<std::uint32_t>(singleBlock.size()) }, oneBlock, parameters, parameters);
        float* splitChannel[] { splitBlocks.data() };
        processDspChorusRamp({ splitChannel, 1, 512 }, split, parameters, parameters);
        float* splitTailChannel[] { splitChannel[0] + 512 };
        processDspChorusRamp({ splitTailChannel, 1, static_cast<std::uint32_t>(splitBlocks.size() - 512) }, split, parameters, parameters);
        require(singleBlock == splitBlocks, "Chorus modulation phase must be deterministic across callback boundaries.");

        std::array<float, 4096> left {}, right {};
        left.front() = right.front() = 1.0f;
        float* stereo[] { left.data(), right.data() };
        DspChorusParameters sweepStart; sweepStart.rateHz = .05; sweepStart.depthMs = .1; sweepStart.baseDelayMs = 5.0; sweepStart.width = 0.0;
        DspChorusParameters sweepEnd; sweepEnd.rateHz = 5.0; sweepEnd.depthMs = 12.0; sweepEnd.baseDelayMs = 30.0; sweepEnd.width = 1.0; sweepEnd.mix = 1.0;
        processDspChorusRamp({ stereo, 2, static_cast<std::uint32_t>(left.size()) }, state, sweepStart, sweepEnd);
        require(std::all_of(left.begin(), left.end(), [](float value) { return std::isfinite(value) && std::abs(value) <= 16.0f; })
                    && std::all_of(right.begin(), right.end(), [](float value) { return std::isfinite(value) && std::abs(value) <= 16.0f; }),
                "Chorus extreme automation must remain finite and bounded.");
        require(std::any_of(left.begin(), left.end(), [](float value) { return std::abs(value) > 1.0e-5f; }),
                "Chorus must emit a delayed modulation response after its bounded delay time.");
        state.reset();
        require(state.phase == 0.0 && state.voices[1].writeIndex == 0, "Chorus reset must restore deterministic phase and write positions.");
        std::cout << "Curated DSP Chorus vectors passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Curated DSP Chorus vectors failed: " << exception.what() << std::endl;
        return 1;
    }
}
