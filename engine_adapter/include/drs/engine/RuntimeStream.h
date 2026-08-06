#pragma once

#include "drs/engine/RuntimeModel.h"

#include <cstdint>
#include <string>

namespace drs::engine
{
std::string getPhase1ReferenceStreamContainerPath();

RuntimeStreamLoadResult loadRuntimeStreamContainer(const std::string& containerPath);
RuntimeStreamLoadResult parseRuntimeStreamContainer(const std::string& text,
                                                    const std::string& containerPath,
                                                    bool validateReferencedPaths = true,
                                                    std::vector<std::uint8_t>* embeddedPayloadBytes = nullptr);
RuntimeStreamLoadResult loadPhase1ReferenceStreamContainer();
RuntimeStreamLoadResult loadRuntimeStreamContainerForInstrument(const RuntimeManifestLoadResult& instrumentResult);

RuntimeStreamReadResult resolveRuntimeStreamRead(const RuntimeStreamContainerModel& container,
                                                 const std::string& sampleId,
                                                 std::uint64_t payloadRelativeOffsetBytes);
} // namespace drs::engine
