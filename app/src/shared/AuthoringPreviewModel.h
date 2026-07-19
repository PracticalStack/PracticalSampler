#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace drs::app
{
struct AuthoringWaveformPreviewPoint
{
    float minValue = 0.0f;
    float maxValue = 0.0f;
};

struct AuthoringWaveformPreview
{
    bool available = false;
    std::string state;
    std::string sourcePath;
    std::string formatName;
    double durationSeconds = 0.0;
    double sampleRate = 0.0;
    std::uint64_t frameCount = 0;
    std::uint32_t channelCount = 0;
    bool loopEnabled = false;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
    std::vector<AuthoringWaveformPreviewPoint> points;
};

struct AuthoringPreviewStatusSnapshot
{
    bool available = false;
    std::size_t draftRevision = 0;
    std::size_t activeRevision = 0;
    std::size_t pendingRevision = 0;
    std::size_t requestedRevision = 0;
    std::size_t failedRevision = 0;
    std::size_t audibleRevision = 0;
    bool auditionAvailable = false;
    bool usingLastKnownGood = false;
    std::string revisionState;
    std::string failureState;
    std::string failureFamily;
    std::string failureCode;
    std::string failurePath;
    std::string blockingPrerequisite;
    std::string blockingGuidance;
};

struct AuthoringImportResponsivenessSnapshot
{
    bool available = false;
    std::string state;
    std::size_t totalItemCount = 0;
    std::size_t pendingCount = 0;
    std::size_t processedCount = 0;
    std::size_t warningItemCount = 0;
    std::size_t failedItemCount = 0;
    std::size_t canceledItemCount = 0;
    std::size_t acceptedItemCount = 0;
    std::uint64_t lastProcessDurationMicros = 0;
    std::uint64_t averageProcessDurationMicros = 0;
    std::uint64_t maxProcessDurationMicros = 0;
    std::string lastProcessedItemId;
};
} // namespace drs::app
