#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace drs::engine
{
// Instrument controls deliberately have their own contract. They are not
// published Performance macros: imported controls may fan out to many audio
// targets without consuming the host-facing macro slots.
inline constexpr int instrumentControlProjectSchemaVersion = 11;
inline constexpr int instrumentControlAuthoringSchemaVersion = 10;
inline constexpr int instrumentControlInstrumentSchemaVersion = 9;
inline constexpr std::size_t maximumInstrumentControls = 128;
inline constexpr std::size_t maximumInstrumentControlContributionsPerRoute = 8;

enum class RuntimeInstrumentControlCategory : std::uint8_t
{
    mixer,
    tuning,
    envelope,
    dynamics,
    tone,
    hidden
};

enum class RuntimeInstrumentControlKind : std::uint8_t
{
    normalized,
    bipolar,
    decibels,
    cents,
    seconds,
    percent,
    stepped,
    toggle
};

enum class RuntimeInstrumentControlUnit : std::uint8_t
{
    generic,
    decibels,
    pan,
    cents,
    seconds,
    percent,
    integer,
    boolean
};

enum class RuntimeInstrumentControlProvenance : std::uint8_t
{
    authored,
    importedSfz,
    migrated
};

enum class RuntimeInstrumentControlTargetKind : std::uint8_t
{
    gain,
    pan,
    tune,
    envelopeHold,
    envelopeDecay,
    envelopeSustain
};

enum class RuntimeInstrumentControlContributionMode : std::uint8_t
{
    multiply,
    add,
    replace
};

enum class RuntimeMidiChannelScopeKind : std::uint8_t
{
    any,
    exact
};

struct RuntimeMidiChannelScope
{
    RuntimeMidiChannelScopeKind kind = RuntimeMidiChannelScopeKind::any;
    std::uint8_t channel = 0; // MIDI channels are stored as 1..16 for exact scope.

    bool operator==(const RuntimeMidiChannelScope& other) const noexcept
    {
        return kind == other.kind && channel == other.channel;
    }
};

struct RuntimeProjectInstrumentControlDefinition
{
    std::string id;
    std::string displayName;
    RuntimeInstrumentControlCategory category = RuntimeInstrumentControlCategory::hidden;
    RuntimeInstrumentControlKind kind = RuntimeInstrumentControlKind::normalized;
    RuntimeInstrumentControlUnit unit = RuntimeInstrumentControlUnit::generic;
    double normalizedDefault = 0.0;
    double displayMinimum = 0.0;
    double displayMaximum = 1.0;
    int displayPrecision = 2;
    int displayOrder = 0;
    std::string section;
    bool visible = true;
    RuntimeInstrumentControlProvenance provenance = RuntimeInstrumentControlProvenance::authored;
    std::optional<int> importedSourceController;
};

struct RuntimeProjectInstrumentControlTargetDefinition
{
    std::string id;
    std::string controlId;
    std::string ownerKind;
    std::string ownerId;
    RuntimeInstrumentControlTargetKind targetKind = RuntimeInstrumentControlTargetKind::gain;
    std::string parameterId;
    double sourceMinimum = 0.0;
    double sourceMaximum = 1.0;
    double destinationMinimum = 0.0;
    double destinationMaximum = 1.0;
    std::string curve = "linear";
    std::array<double, 128> curvePoints {};
    int curveIndex = -1;
    RuntimeInstrumentControlContributionMode contributionMode
        = RuntimeInstrumentControlContributionMode::replace;
};

struct RuntimeProjectMidiControlBindingDefinition
{
    std::string id;
    std::string controlId;
    int controllerNumber = 0;
    RuntimeMidiChannelScope channelScope;
    bool enabled = true;
    bool imported = false;
    std::optional<int> importedSourceController;
};

struct RuntimeInstrumentControlValidationResult
{
    bool valid = false;
    std::vector<std::string> issues;
};

RuntimeInstrumentControlValidationResult validateInstrumentControlCatalog(
    const std::vector<RuntimeProjectInstrumentControlDefinition>& controls,
    const std::vector<RuntimeProjectInstrumentControlTargetDefinition>& targets,
    const std::vector<RuntimeProjectMidiControlBindingDefinition>& bindings);

double resolveImportedSfzControlDefault(
    const RuntimeProjectInstrumentControlDefinition& control,
    const std::vector<RuntimeProjectInstrumentControlTargetDefinition>& targets,
    bool hasExplicitControllerDefault) noexcept;

const char* runtimeInstrumentControlCategoryName(RuntimeInstrumentControlCategory value) noexcept;
const char* runtimeInstrumentControlKindName(RuntimeInstrumentControlKind value) noexcept;
const char* runtimeInstrumentControlUnitName(RuntimeInstrumentControlUnit value) noexcept;
const char* runtimeInstrumentControlProvenanceName(RuntimeInstrumentControlProvenance value) noexcept;
const char* runtimeInstrumentControlTargetKindName(RuntimeInstrumentControlTargetKind value) noexcept;
const char* runtimeInstrumentControlContributionModeName(RuntimeInstrumentControlContributionMode value) noexcept;
const char* runtimeMidiChannelScopeKindName(RuntimeMidiChannelScopeKind value) noexcept;

bool parseRuntimeInstrumentControlCategory(const std::string& value,
                                          RuntimeInstrumentControlCategory& result) noexcept;
bool parseRuntimeInstrumentControlKind(const std::string& value,
                                       RuntimeInstrumentControlKind& result) noexcept;
bool parseRuntimeInstrumentControlUnit(const std::string& value,
                                       RuntimeInstrumentControlUnit& result) noexcept;
bool parseRuntimeInstrumentControlProvenance(const std::string& value,
                                             RuntimeInstrumentControlProvenance& result) noexcept;
bool parseRuntimeInstrumentControlTargetKind(const std::string& value,
                                             RuntimeInstrumentControlTargetKind& result) noexcept;
bool parseRuntimeInstrumentControlContributionMode(
    const std::string& value,
    RuntimeInstrumentControlContributionMode& result) noexcept;
bool parseRuntimeMidiChannelScopeKind(const std::string& value,
                                      RuntimeMidiChannelScopeKind& result) noexcept;
} // namespace drs::engine
