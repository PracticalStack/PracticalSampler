#pragma once

#include "drs/engine/RuntimeModel.h"

#include <string>
#include <string_view>
#include <vector>

namespace drs::engine
{
struct PerformanceRuleFinding
{
    std::string code;
    std::string path;
    std::string message;
    std::string repair;
};

struct PerformanceRuleValidationResult
{
    bool valid = false;
    std::vector<PerformanceRuleFinding> findings;
};

std::string_view performanceEventKindId(PerformanceEventKind value) noexcept;
std::string_view performanceSustainConditionId(PerformanceSustainCondition value) noexcept;
std::string_view performancePitchSourceId(PerformancePitchSource value) noexcept;
std::string_view articulationActivationModeId(ArticulationActivationMode value) noexcept;
std::string_view roundRobinResetEventId(RoundRobinResetEvent value) noexcept;

bool parsePerformanceEventKind(std::string_view value, PerformanceEventKind& result) noexcept;
bool parsePerformanceSustainCondition(std::string_view value, PerformanceSustainCondition& result) noexcept;
bool parsePerformancePitchSource(std::string_view value, PerformancePitchSource& result) noexcept;
bool parseArticulationActivationMode(std::string_view value, ArticulationActivationMode& result) noexcept;
bool parseRoundRobinResetEvent(std::string_view value, RoundRobinResetEvent& result) noexcept;

PerformanceRuleValidationResult validatePerformanceRuleDeclarations(
    const RuntimeProjectAuthoringState& authoring);
} // namespace drs::engine
