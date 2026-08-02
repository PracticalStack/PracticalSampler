#include "drs/engine/PerformanceRuleContract.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
using namespace drs::engine;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool hasFinding(const PerformanceRuleValidationResult& result, const std::string& code)
{
    return std::any_of(result.findings.begin(), result.findings.end(), [&](const auto& finding)
    {
        return finding.code == code;
    });
}

RuntimeProjectModel makeRuleProject()
{
    const auto loaded = loadRuntimeProjectManifest(DRS_PERFORMANCE_ENGINE_S2_LEGACY_FIXTURE_PATH);
    require(loaded.loaded, "Sprint 2 must begin from a valid schema-5 project.");
    auto legacy = loaded.project;
    for (std::size_t index = 0; index < legacy.authoring.zones.size(); ++index)
    {
        legacy.authoring.zones[index].articulationId = index == 1 ? "staccato" : "sustain";
        legacy.authoring.zones[index].keyLow = 36;
        legacy.authoring.zones[index].keyHigh = 100;
    }
    const auto migration = migrateRuntimeProjectToPerformanceArticulationSchema(legacy);
    require(migration.valid, "Schema-5 fixture must migrate to schema 6/5 before rule coverage.");
    auto project = migration.project;
    require(project.authoring.zones.size() >= 3, "Rule fixture requires three zones.");

    auto& sustain = project.authoring.articulations[0];
    auto& staccato = project.authoring.articulations[1];
    sustain.activation = RuntimeProjectArticulationActivationDefinition {
        PerformanceEventKind::noteOn, 12, ArticulationActivationMode::latch, true };
    staccato.activation = RuntimeProjectArticulationActivationDefinition {
        PerformanceEventKind::noteOn, 14, ArticulationActivationMode::latch, true };

    auto& main = project.authoring.zones[0];
    main.performance = { PerformanceEventKind::noteOn, PerformanceSustainCondition::any,
                         PerformancePitchSource::eventNote };
    main.exclusiveGroupId = "main-voice";
    main.exclusiveTargetGroupIds = { "release-noise" };
    main.chokeReleaseSeconds = 0.025;

    auto& release = project.authoring.zones[1];
    release.triggerMode = ZoneTriggerMode::oneShot;
    release.performance = { PerformanceEventKind::release, PerformanceSustainCondition::pedalUp,
                            PerformancePitchSource::eventNote };
    release.exclusiveGroupId = "release-noise";

    auto& pedal = project.authoring.zones[2];
    pedal.triggerMode = ZoneTriggerMode::oneShot;
    pedal.performance = { PerformanceEventKind::pedalUp, PerformanceSustainCondition::pedalUp,
                          PerformancePitchSource::fixedRoot };
    pedal.rootKey = 36;

    project.authoring.roundRobinResetRules = {
        { RoundRobinResetEvent::articulationChange, true, {} },
        { RoundRobinResetEvent::pedalUp, true, {} }
    };
    return project;
}

void verifyTypedRoundTrip()
{
    const auto project = makeRuleProject();
    const auto validation = validateRuntimeProjectModel(project);
    require(validation.valid, "A closed-vocabulary declaration set must validate: "
                              + (validation.issues.empty() ? std::string {} : validation.issues.front()));
    const auto serialized = serializeRuntimeProjectManifest(project, "performance-engine-sprint2.drsproj");
    require(serialized.find("\"performance\"") != std::string::npos
                && serialized.find("\"roundRobinResetRules\"") != std::string::npos
                && serialized.find("\"exclusiveTargetGroupIds\"") != std::string::npos,
            "Schema-6 serialization must persist every declarative rule family.");
    const auto parsed = parseRuntimeProjectManifest(serialized, "performance-engine-sprint2.drsproj", false);
    require(parsed.loaded, "Typed performance declarations must reload.");
    require(parsed.project.authoring.zones[1].performance.event == PerformanceEventKind::release
                && parsed.project.authoring.zones[2].performance.pitchSource == PerformancePitchSource::fixedRoot
                && parsed.project.authoring.articulations[0].activation.has_value()
                && parsed.project.authoring.articulations[0].activation->midiNote == 12
                && parsed.project.authoring.roundRobinResetRules.size() == 2,
            "Reload must preserve typed event, pitch, activation, and RR declarations.");
    require(serializeRuntimeProjectManifest(parsed.project, "performance-engine-sprint2.drsproj") == serialized,
            "Typed declarative rules must round-trip byte deterministically.");
}

void verifyActionableFailures()
{
    auto project = makeRuleProject();
    project.authoring.zones[0].keyLow = 0;
    auto findings = validatePerformanceRuleDeclarations(project.authoring);
    require(hasFinding(findings, "performance.activation.playable_collision"),
            "Key switches must reject playable-range collisions.");

    project = makeRuleProject();
    project.authoring.zones[1].triggerMode = ZoneTriggerMode::gated;
    findings = validatePerformanceRuleDeclarations(project.authoring);
    require(hasFinding(findings, "performance.zone.release_recursion"),
            "Gated release routes must reject recursion-prone declarations.");

    project = makeRuleProject();
    project.authoring.zones[0].performance.pitchSource = PerformancePitchSource::fixedRoot;
    findings = validatePerformanceRuleDeclarations(project.authoring);
    require(hasFinding(findings, "performance.zone.fixed_pitch_invalid"),
            "Fixed-root note routes must be rejected in v1.");

    project = makeRuleProject();
    project.authoring.zones[0].exclusiveTargetGroupIds = { "missing-group" };
    findings = validatePerformanceRuleDeclarations(project.authoring);
    require(hasFinding(findings, "performance.choke.target_unknown"),
            "Choke targets must name an existing stable exclusive group.");

    project = makeRuleProject();
    project.authoring.roundRobinResetRules[1] = { RoundRobinResetEvent::pedalUp, false, "missing-pool" };
    findings = validatePerformanceRuleDeclarations(project.authoring);
    require(hasFinding(findings, "performance.rr_reset.target_unknown"),
            "RR reset rules must name an existing pool unless they target all pools.");

    project = makeRuleProject();
    project.authoring.articulations[1].activation->midiNote = 12;
    findings = validatePerformanceRuleDeclarations(project.authoring);
    require(hasFinding(findings, "performance.activation.duplicate_note"),
            "Latch activation notes must remain unambiguous.");
}
} // namespace

int main()
{
    try
    {
        verifyTypedRoundTrip();
        verifyActionableFailures();
        std::cout << "Performance-engine Sprint 2 declarative-rule tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance-engine Sprint 2 declarative-rule tests failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
