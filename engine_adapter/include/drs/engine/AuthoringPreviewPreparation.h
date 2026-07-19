#pragma once

#include "drs/engine/AuthoringPreviewContract.h"
#include "drs/engine/SamplerRenderModel.h"

#include <cstddef>
#include <vector>

namespace drs::engine
{
struct AuthoringPreviewPreparationResult
{
    bool prepared = false;
    AuthoringPreviewScope scope = AuthoringPreviewScope::selectedZone;
    PlaybackActivationPayloadPtr scopedPayload;
    SamplerRenderModelPtr model;
    std::vector<SamplerRenderModelFinding> findings;
    std::size_t validatedZoneCount = 0;
    std::size_t retainedZoneCount = 0;
    std::size_t retainedSampleCount = 0;
};

// Message-owned normalization boundary. The complete worker payload is validated before
// a selected-zone request is filtered, so invalid project topology cannot hide outside the route.
AuthoringPreviewPreparationResult prepareAuthoringPreviewRenderModel(
    const PlaybackActivationPayloadPtr& preparedDraftPayload,
    const AuthoringPreviewRequest& request);
} // namespace drs::engine
