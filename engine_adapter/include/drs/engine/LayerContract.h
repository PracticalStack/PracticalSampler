#pragma once

#include <cstdint>
#include <optional>

namespace drs::engine
{
// Layer contract schema versions. A layer is a first-class parent of groups;
// these versions advance the project and authoring schemas together.
inline constexpr int layerContractProjectSchemaVersion = 10;
inline constexpr int layerContractAuthoringSchemaVersion = 9;

enum class LayerCrossfadeSource : std::uint8_t
{
    none,
    velocity,
    controller
};

enum class LayerCrossfadeCurve : std::uint8_t
{
    linear
};

enum class LayerCrossfadeDirection : std::uint8_t
{
    fadeIn,
    fadeOut
};

struct RuntimeProjectLayerCrossfadeDefinition
{
    LayerCrossfadeSource source = LayerCrossfadeSource::none;
    std::optional<int> controllerNumber;
    int low = 0;
    int high = 127;
    LayerCrossfadeCurve curve = LayerCrossfadeCurve::linear;
    LayerCrossfadeDirection direction = LayerCrossfadeDirection::fadeIn;
};
} // namespace drs::engine
