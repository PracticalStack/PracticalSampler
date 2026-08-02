#pragma once

#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/ControlLaw.h"
#include "drs/engine/DspParameterControl.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace drs::engine
{
// Product capacity is deliberately separate from the physical host topology.  The
// four remaining slots are reserved for hidden helpers and compatibility; they do
// not make additional player-facing controls available.
constexpr std::size_t maximumExposedPerformanceControls = 12;
constexpr std::size_t maximumPublishedMacroHostSlots = 16;

struct PublishedMacroHostTopologySlot
{
    std::size_t slotIndex = 0;
    const char* hostParameterId = "";
    const char* hostParameterName = "";
    double defaultValue = 0.5;
};

// This is the compatibility contract for the permanent host topology.  The
// first two identifiers are already visible to existing sessions, while the
// remaining identifiers use one-based product slot numbering.  Authored macro
// identities are intentionally not stored here: publication binds them later.
inline const std::array<PublishedMacroHostTopologySlot, maximumPublishedMacroHostSlots>&
publishedMacroHostTopology()
{
    static const std::array<PublishedMacroHostTopologySlot, maximumPublishedMacroHostSlots>
        topology {{
            { 0, "macro.tone", "Tone", 0.35 },
            { 1, "macro.motion", "Motion", 0.15 },
            { 2, "macro.slot.3", "Performance Slot 3", 0.5 },
            { 3, "macro.slot.4", "Performance Slot 4", 0.5 },
            { 4, "macro.slot.5", "Performance Slot 5", 0.5 },
            { 5, "macro.slot.6", "Performance Slot 6", 0.5 },
            { 6, "macro.slot.7", "Performance Slot 7", 0.5 },
            { 7, "macro.slot.8", "Performance Slot 8", 0.5 },
            { 8, "macro.slot.9", "Performance Slot 9", 0.5 },
            { 9, "macro.slot.10", "Performance Slot 10", 0.5 },
            { 10, "macro.slot.11", "Performance Slot 11", 0.5 },
            { 11, "macro.slot.12", "Performance Slot 12", 0.5 },
            { 12, "macro.slot.13", "Performance Slot 13", 0.5 },
            { 13, "macro.slot.14", "Performance Slot 14", 0.5 },
            { 14, "macro.slot.15", "Performance Slot 15", 0.5 },
            { 15, "macro.slot.16", "Performance Slot 16", 0.5 }
        }};
    return topology;
}

static_assert(maximumPublishedMacroHostSlots - maximumExposedPerformanceControls == 4,
              "Performance controls reserve exactly four hidden-helper/compatibility slots.");

enum class PublishedMacroRenderTarget : std::uint8_t
{
    none = 0,
    toneVelocity,
    motionPitch,
    dspControl
};

// Presentation is prepared with the published table. The audio callback only
// receives PublishedMacroCallbackView, so none of these strings enter the
// realtime path.
enum class PublishedMacroControlKind : std::uint8_t
{
    knob = 0,
    fader,
    toggle
};

struct PublishedMacroPresentation
{
    std::string authoredLabel;
    std::string sectionLabel;
    std::string parameterLabel;
    std::string valueUnit;
    PublishedMacroControlKind controlKind = PublishedMacroControlKind::knob;
    std::size_t authoredOrder = 0;
    std::string accessibilityDescription;
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
    std::uint32_t dspControlIndex = 0;
    double sourceMinimum = 0.0;
    double sourceMaximum = 1.0;
    double destinationMinimum = 0.0;
    double destinationMaximum = 1.0;
    // Bounded, allocation-free law payload used directly by the audio callback.
    CompiledControlLaw controlLaw;
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
    PublishedMacroPresentation presentation;
    double minValue = 0.0;
    double maxValue = 1.0;
    double defaultValue = 0.0;
    double publishedValue = 0.0;
    bool assigned = false;
    bool exposedInPerformance = false;
    PublishedMacroRenderTarget renderTarget = PublishedMacroRenderTarget::none;
    std::uint32_t dspControlIndex = 0;
    std::string dspSlotId;
    std::string dspParameterId;
    double sourceMinimum = 0.0;
    double sourceMaximum = 1.0;
    double destinationMinimum = 0.0;
    double destinationMaximum = 1.0;
    CompiledControlLaw controlLaw;
};

struct ImmutablePublishedMacroBindingTable final
{
    std::size_t revision = 0;
    std::string macroSchemaDigest;
    std::string dspGraphDigest;
    std::vector<PublishedMacroBinding> bindings;
    std::vector<std::string> retiredStableAuthoredIds;
    std::vector<std::string> unassignedStableAuthoredIds;
    std::size_t assignedExposedCount = 0;
    std::size_t assignedHiddenCount = 0;
    std::size_t unassignedExposedCount = 0;
    std::size_t unassignedHiddenCount = 0;
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
    // Derived from the immutable playback snapshot by EngineFacade; no UI
    // surface needs to reconstruct source ownership from authoring state.
    struct PresentationHint
    {
        std::string stableAuthoredId;
        PublishedMacroPresentation presentation;
    };
    std::vector<PresentationHint> presentationHints;
    std::vector<PublishedMacroCurrentValue> currentValues;
    ImmutablePublishedMacroBindingTablePtr previousActiveTable;
    const DspParameterControlLayout* dspControlLayout = nullptr;
    std::string dspGraphDigest;
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
