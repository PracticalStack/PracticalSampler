#pragma once

#include "drs/engine/PlaybackSnapshot.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace drs::engine
{
constexpr std::size_t maximumPublishedMacroHostSlots = 16;

enum class PublishedMacroRenderTarget : std::uint8_t
{
    none = 0,
    toneVelocity,
    motionPitch
};

struct PublishedMacroHostSlotDefinition
{
    std::size_t slotIndex = 0;
    std::string hostParameterId;
    std::string stableAuthoredId;
};

struct PublishedMacroCurrentValue
{
    std::string stableAuthoredId;
    double value = 0.0;
};

struct PublishedMacroCallbackSlot
{
    bool assigned = false;
    PublishedMacroRenderTarget renderTarget = PublishedMacroRenderTarget::none;
    double minValue = 0.0;
    double maxValue = 1.0;
    double publishedValue = 0.0;
};

struct PublishedMacroCallbackView
{
    std::size_t revision = 0;
    std::size_t hostSlotCount = 0;
    std::array<PublishedMacroCallbackSlot, maximumPublishedMacroHostSlots> slots {};
};

struct PublishedMacroBinding
{
    std::size_t hostSlotIndex = 0;
    std::string hostParameterId;
    std::string stableAuthoredId;
    std::string publishedName;
    double minValue = 0.0;
    double maxValue = 1.0;
    double defaultValue = 0.0;
    double publishedValue = 0.0;
    bool assigned = false;
    PublishedMacroRenderTarget renderTarget = PublishedMacroRenderTarget::none;
};

struct ImmutablePublishedMacroBindingTable final
{
    std::size_t revision = 0;
    std::string macroSchemaDigest;
    std::vector<PublishedMacroBinding> bindings;
    std::vector<std::string> retiredStableAuthoredIds;
    std::vector<std::string> unassignedStableAuthoredIds;
    PublishedMacroCallbackView callbackView;
};

using ImmutablePublishedMacroBindingTablePtr
    = std::shared_ptr<const ImmutablePublishedMacroBindingTable>;

enum class PublishedMacroBindingFindingSeverity : std::uint8_t
{
    warning = 0,
    error
};

struct PublishedMacroBindingFinding
{
    PublishedMacroBindingFindingSeverity severity = PublishedMacroBindingFindingSeverity::error;
    std::string code;
    std::string path;
    std::string message;
};

struct PublishedMacroBindingBuildRequest
{
    std::size_t revision = 0;
    std::string macroSchemaDigest;
    std::vector<PublishedMacroHostSlotDefinition> hostSlots;
    std::vector<PlaybackSnapshotMacroDefault> authoredMacros;
    std::vector<PublishedMacroCurrentValue> currentValues;
    ImmutablePublishedMacroBindingTablePtr previousActiveTable;
};

struct PublishedMacroBindingBuildResult
{
    bool built = false;
    ImmutablePublishedMacroBindingTablePtr table;
    std::vector<PublishedMacroBindingFinding> findings;
};

PublishedMacroBindingBuildResult buildPublishedMacroBindingTable(
    const PublishedMacroBindingBuildRequest& request);
} // namespace drs::engine
