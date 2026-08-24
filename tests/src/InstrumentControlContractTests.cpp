#include "drs/engine/InstrumentControlContract.h"

#include <iostream>
#include <stdexcept>

namespace
{
void require(const bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

drs::engine::RuntimeProjectInstrumentControlDefinition makeControl(const char* id)
{
    drs::engine::RuntimeProjectInstrumentControlDefinition control;
    control.id = id;
    control.displayName = id;
    control.category = drs::engine::RuntimeInstrumentControlCategory::mixer;
    control.kind = drs::engine::RuntimeInstrumentControlKind::decibels;
    control.unit = drs::engine::RuntimeInstrumentControlUnit::decibels;
    control.normalizedDefault = 0.5;
    control.displayMinimum = -96.0;
    control.displayMaximum = 6.0;
    return control;
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        auto control = makeControl("mixer.kick");
        RuntimeProjectInstrumentControlTargetDefinition target;
        target.id = "target.kick.gain";
        target.controlId = control.id;
        target.ownerKind = "group";
        target.ownerId = "kick";
        target.targetKind = RuntimeInstrumentControlTargetKind::gain;
        target.parameterId = "gainDb";
        target.destinationMinimum = -96.0;
        target.destinationMaximum = 6.0;
        target.contributionMode = RuntimeInstrumentControlContributionMode::multiply;

        RuntimeProjectMidiControlBindingDefinition binding;
        binding.id = "binding.kick.cc20";
        binding.controlId = control.id;
        binding.controllerNumber = 20;
        binding.imported = true;

        const auto valid = validateInstrumentControlCatalog({ control }, { target }, { binding });
        require(valid.valid, "A complete control/target/binding catalog must validate.");
        require(std::string(runtimeInstrumentControlCategoryName(control.category)) == "mixer"
                    && std::string(runtimeInstrumentControlTargetKindName(target.targetKind)) == "gain"
                    && std::string(runtimeMidiChannelScopeKindName(binding.channelScope.kind)) == "any",
                "Versioned control enum names must remain stable for persistence and diagnostics.");

        auto duplicate = binding;
        duplicate.id = "binding.kick.cc20.channel2";
        duplicate.channelScope.kind = RuntimeMidiChannelScopeKind::exact;
        duplicate.channelScope.channel = 2;
        const auto conflicting = validateInstrumentControlCatalog(
            { control }, { target }, { binding, duplicate });
        require(!conflicting.valid && !conflicting.issues.empty(),
                "An Any Channel binding must conflict with an exact-channel binding for the same CC.");

        auto invalidControl = control;
        invalidControl.normalizedDefault = 1.5;
        const auto invalid = validateInstrumentControlCatalog(
            { invalidControl }, { target }, { binding });
        require(!invalid.valid, "Out-of-range normalized defaults must be rejected.");

        std::cout << "Instrument control contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Instrument control contract tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
