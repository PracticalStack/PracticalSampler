#include "drs/engine/RuntimeStream.h"

#include "drs/engine/WorkspacePaths.generated.h"

#include <json/json.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;
using json = nlohmann::json;

void addIssue(RuntimeStreamLoadResult& result, const std::string& issue)
{
    result.issues.push_back(issue);
}

std::string toDisplayPath(const fs::path& path)
{
    return path.lexically_normal().generic_string();
}

std::string readTextFile(const fs::path& filePath)
{
    std::ifstream input(filePath, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

fs::path resolveRelativePath(const fs::path& containerPath, const std::string& rawPath)
{
    const fs::path candidate(rawPath);

    if (candidate.is_absolute())
        return candidate.lexically_normal();

    return (containerPath.parent_path() / candidate).lexically_normal();
}

template <typename TValue>
std::optional<TValue> readRequired(const json& object,
                                   RuntimeStreamLoadResult& result,
                                   const char* propertyName,
                                   const char* context)
{
    const auto iterator = object.find(propertyName);

    if (iterator == object.end())
    {
        addIssue(result, std::string(context) + " is missing required field '" + propertyName + "'.");
        return std::nullopt;
    }

    try
    {
        return iterator->get<TValue>();
    }
    catch (const json::exception&)
    {
        addIssue(result, std::string(context) + " has invalid type for field '" + propertyName + "'.");
        return std::nullopt;
    }
}

std::vector<std::string> readRequiredStringArray(const json& object,
                                                 RuntimeStreamLoadResult& result,
                                                 const char* propertyName,
                                                 const char* context)
{
    std::vector<std::string> values;

    const auto iterator = object.find(propertyName);
    if (iterator == object.end())
    {
        addIssue(result, std::string(context) + " is missing required field '" + propertyName + "'.");
        return values;
    }

    if (!iterator->is_array())
    {
        addIssue(result, std::string(context) + " field '" + propertyName + "' must be an array.");
        return values;
    }

    values.reserve(iterator->size());
    for (const auto& entry : *iterator)
    {
        if (!entry.is_string())
        {
            addIssue(result, std::string(context) + " field '" + propertyName + "' must contain only strings.");
            continue;
        }

        values.push_back(entry.get<std::string>());
    }

    return values;
}

bool isObjectArray(const json& value)
{
    return value.is_array()
        && std::all_of(value.begin(), value.end(), [](const auto& entry) { return entry.is_object(); });
}

std::string computeFnv1aChecksumHex(const fs::path& path)
{
    constexpr std::uint64_t offsetBasis = 14695981039346656037ull;
    constexpr std::uint64_t prime = 1099511628211ull;

    std::ifstream input(path, std::ios::binary);
    if (!input.good())
        return {};

    std::uint64_t hash = offsetBasis;
    char buffer[4096];

    while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0)
    {
        for (std::streamsize index = 0; index < input.gcount(); ++index)
        {
            hash ^= static_cast<unsigned char>(buffer[index]);
            hash *= prime;
        }
    }

    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return stream.str();
}

std::string computeFnv1aChecksumHexForRange(const fs::path& path,
                                            const std::uint64_t offsetBytes,
                                            const std::uint64_t sizeBytes)
{
    constexpr std::uint64_t offsetBasis = 14695981039346656037ull;
    constexpr std::uint64_t prime = 1099511628211ull;

    std::ifstream input(path, std::ios::binary);
    if (!input.good())
        return {};

    input.seekg(static_cast<std::streamoff>(offsetBytes), std::ios::beg);
    if (!input.good())
        return {};

    std::uint64_t hash = offsetBasis;
    std::array<char, 4096> buffer {};
    auto remainingBytes = sizeBytes;

    while (remainingBytes > 0)
    {
        const auto chunkBytes = static_cast<std::streamsize>(
            std::min<std::uint64_t>(remainingBytes, static_cast<std::uint64_t>(buffer.size())));
        input.read(buffer.data(), chunkBytes);
        if (!input.good() && input.gcount() != chunkBytes)
            return {};

        for (std::streamsize index = 0; index < chunkBytes; ++index)
        {
            hash ^= static_cast<unsigned char>(buffer[static_cast<std::size_t>(index)]);
            hash *= prime;
        }

        remainingBytes -= static_cast<std::uint64_t>(chunkBytes);
    }

    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return stream.str();
}

std::string computeFnv1aChecksumHexForRange(const std::vector<std::uint8_t>& bytes,
                                            const std::uint64_t offsetBytes,
                                            const std::uint64_t sizeBytes)
{
    constexpr std::uint64_t offsetBasis = 14695981039346656037ull;
    constexpr std::uint64_t prime = 1099511628211ull;

    if (offsetBytes > bytes.size() || sizeBytes > bytes.size() - offsetBytes)
        return {};

    std::uint64_t hash = offsetBasis;
    for (std::uint64_t index = 0; index < sizeBytes; ++index)
    {
        hash ^= bytes[static_cast<std::size_t>(offsetBytes + index)];
        hash *= prime;
    }

    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return stream.str();
}

std::string inferChannelLayout(std::uint32_t channelCount)
{
    switch (channelCount)
    {
    case 0:
        return {};
    case 1:
        return "mono";
    case 2:
        return "stereo";
    default:
        return std::to_string(channelCount) + " channels";
    }
}

std::uint64_t alignUp(std::uint64_t value, std::uint64_t alignment)
{
    if (alignment == 0)
        return value;

    const auto remainder = value % alignment;
    return remainder == 0 ? value : value + (alignment - remainder);
}
} // namespace

std::string getPhase1ReferenceStreamContainerPath()
{
    return generated::workspacePhase1ReferenceStream;
}

RuntimeStreamLoadResult parseRuntimeStreamContainer(const std::string& rawText,
                                                    const std::string& containerPath,
                                                    const bool validateReferencedPaths,
                                                    const std::vector<std::uint8_t>* embeddedPayloadBytes)
{
    RuntimeStreamLoadResult result;
    result.containerPath = containerPath;
    result.state = "Stream-container parse not attempted";
    result.containerFound = true;

    const fs::path containerFsPath(containerPath);
    if (rawText.empty())
    {
        result.state = "Stream-container unreadable";
        addIssue(result, "Stream-container JSON text was empty.");
        return result;
    }

    json root;
    try
    {
        root = json::parse(rawText);
    }
    catch (const json::exception& exception)
    {
        result.state = "Stream-container parse failed";
        addIssue(result, "Stream-container JSON parse failed: " + std::string(exception.what()));
        return result;
    }

    if (!root.is_object())
    {
        result.state = "Stream-container root invalid";
        addIssue(result, "Stream-container root must be a JSON object.");
        return result;
    }

    auto& container = result.container;

    if (const auto schemaName = readRequired<std::string>(root, result, "schemaName", "StreamContainer"))
        container.schemaName = *schemaName;

    if (const auto schemaVersion = readRequired<int>(root, result, "schemaVersion", "StreamContainer"))
        container.schemaVersion = *schemaVersion;

    if (const auto containerId = readRequired<std::string>(root, result, "containerId", "StreamContainer"))
        container.containerId = *containerId;

    if (const auto pageSizeBytes = readRequired<std::uint64_t>(root, result, "pageSizeBytes", "StreamContainer"))
        container.pageSizeBytes = *pageSizeBytes;

    if (const auto payloadEncoding = readRequired<std::string>(root, result, "payloadEncoding", "StreamContainer"))
        container.payloadEncoding = *payloadEncoding;

    if (const auto totalPayloadBytes = readRequired<std::uint64_t>(root, result, "totalPayloadBytes", "StreamContainer"))
        container.totalPayloadBytes = *totalPayloadBytes;

    if (const auto payloadAssetIterator = root.find("payloadAssetPath"); payloadAssetIterator != root.end())
    {
        if (!payloadAssetIterator->is_string())
        {
            addIssue(result, "StreamContainer field 'payloadAssetPath' must be a string when present.");
        }
        else
        {
            container.payloadAssetPath = validateReferencedPaths
                ? toDisplayPath(resolveRelativePath(containerFsPath,
                                                    payloadAssetIterator->get<std::string>()))
                : payloadAssetIterator->get<std::string>();
        }
    }

    if (const auto payloadFileBytesIterator = root.find("payloadFileBytes"); payloadFileBytesIterator != root.end())
    {
        if (!payloadFileBytesIterator->is_number_unsigned())
        {
            addIssue(result, "StreamContainer field 'payloadFileBytes' must be an unsigned integer when present.");
        }
        else
        {
            container.payloadFileBytes = payloadFileBytesIterator->get<std::uint64_t>();
        }
    }

    if (const auto payloadFileChecksumIterator = root.find("payloadFileChecksumHex"); payloadFileChecksumIterator != root.end())
    {
        if (!payloadFileChecksumIterator->is_string())
        {
            addIssue(result, "StreamContainer field 'payloadFileChecksumHex' must be a string when present.");
        }
        else
        {
            container.payloadFileChecksumHex = payloadFileChecksumIterator->get<std::string>();
        }
    }

    const auto declaredSampleCount = readRequired<std::size_t>(root, result, "sampleCount", "StreamContainer");

    const auto samplesIterator = root.find("samples");
    if (samplesIterator == root.end() || !isObjectArray(*samplesIterator))
    {
        addIssue(result, "StreamContainer field 'samples' must be an array of objects.");
    }
    else
    {
        container.samples.reserve(samplesIterator->size());
        std::uint64_t previousAlignedEnd = 0;
        std::uint64_t accumulatedPayloadBytes = 0;

        for (std::size_t sampleIndex = 0; sampleIndex < samplesIterator->size(); ++sampleIndex)
        {
            const auto& sampleObject = samplesIterator->at(sampleIndex);
            const auto context = "StreamSample[" + std::to_string(sampleIndex) + "]";
            RuntimeStreamSampleDefinition sample;

            if (const auto sampleId = readRequired<std::string>(sampleObject, result, "sampleId", context.c_str()))
                sample.sampleId = *sampleId;

            if (const auto sourceChecksumHex = readRequired<std::string>(sampleObject, result, "sourceChecksumHex", context.c_str()))
                sample.sourceChecksumHex = *sourceChecksumHex;

            if (const auto sourcePath = readRequired<std::string>(sampleObject, result, "sourcePath", context.c_str()))
            {
                if (validateReferencedPaths)
                {
                    std::error_code errorCode;
                    const auto resolvedSourcePath = resolveRelativePath(containerFsPath, *sourcePath);
                    sample.sourcePath = toDisplayPath(resolvedSourcePath);

                    if (!fs::exists(resolvedSourcePath, errorCode))
                    {
                        addIssue(result, context + " source sample does not exist: " + sample.sourcePath);
                    }
                    else
                    {
                        const auto actualChecksum = computeFnv1aChecksumHex(resolvedSourcePath);
                        if (actualChecksum.empty())
                        {
                            addIssue(result, context + " could not compute source checksum for " + sample.sourcePath + ".");
                        }
                        else if (actualChecksum != sample.sourceChecksumHex)
                        {
                            addIssue(result,
                                     context + " checksum mismatch for source sample " + sample.sourcePath
                                         + ": expected " + sample.sourceChecksumHex + ", actual " + actualChecksum + ".");
                        }
                        else
                        {
                            ++result.metrics.checksumValidatedCount;
                        }
                    }
                }
                else
                {
                    sample.sourcePath = *sourcePath;
                }
            }

            if (const auto formatName = readRequired<std::string>(sampleObject, result, "formatName", context.c_str()))
                sample.formatName = *formatName;

            if (const auto payloadChecksumIterator = sampleObject.find("payloadChecksumHex");
                payloadChecksumIterator != sampleObject.end())
            {
                if (!payloadChecksumIterator->is_string())
                {
                    addIssue(result, context + " field 'payloadChecksumHex' must be a string when present.");
                }
                else
                {
                    sample.payloadChecksumHex = payloadChecksumIterator->get<std::string>();
                }
            }

            if (const auto role = readRequired<std::string>(sampleObject, result, "role", context.c_str()))
                sample.role = *role;

            if (const auto sampleRate = readRequired<double>(sampleObject, result, "sampleRate", context.c_str()))
                sample.sampleRate = *sampleRate;

            if (const auto frameCount = readRequired<std::uint64_t>(sampleObject, result, "frameCount", context.c_str()))
                sample.frameCount = *frameCount;

            if (const auto channelCount = readRequired<std::uint32_t>(sampleObject, result, "channelCount", context.c_str()))
                sample.channelCount = *channelCount;

            if (const auto channelLayoutIterator = sampleObject.find("channelLayout");
                channelLayoutIterator != sampleObject.end())
            {
                if (!channelLayoutIterator->is_string())
                {
                    addIssue(result, context + " field 'channelLayout' must be a string when present.");
                }
                else
                {
                    sample.channelLayout = channelLayoutIterator->get<std::string>();
                }
            }

            if (sample.channelLayout.empty())
                sample.channelLayout = inferChannelLayout(sample.channelCount);

            if (const auto payloadOffsetBytes = readRequired<std::uint64_t>(sampleObject, result, "payloadOffsetBytes", context.c_str()))
                sample.payloadOffsetBytes = *payloadOffsetBytes;

            if (const auto payloadSizeBytes = readRequired<std::uint64_t>(sampleObject, result, "payloadSizeBytes", context.c_str()))
                sample.payloadSizeBytes = *payloadSizeBytes;

            if (const auto prefetchBytes = readRequired<std::uint64_t>(sampleObject, result, "prefetchBytes", context.c_str()))
                sample.prefetchBytes = *prefetchBytes;

            if (const auto rootMidiNoteIterator = sampleObject.find("rootMidiNote"); rootMidiNoteIterator != sampleObject.end())
            {
                if (!rootMidiNoteIterator->is_number_integer())
                {
                    addIssue(result, context + " field 'rootMidiNote' must be an integer when present.");
                }
                else
                {
                    sample.rootMidiNotePresent = true;
                    sample.rootMidiNote = rootMidiNoteIterator->get<int>();
                }
            }

            const auto loopStartIterator = sampleObject.find("loopStartFrame");
            const auto loopEndIterator = sampleObject.find("loopEndFrame");
            if (loopStartIterator != sampleObject.end() || loopEndIterator != sampleObject.end())
            {
                if (loopStartIterator == sampleObject.end() || loopEndIterator == sampleObject.end()
                    || !loopStartIterator->is_number_unsigned() || !loopEndIterator->is_number_unsigned())
                {
                    addIssue(result, context + " loop metadata must include unsigned 'loopStartFrame' and 'loopEndFrame' together.");
                }
                else
                {
                    sample.loopRangePresent = true;
                    sample.loopStartFrame = loopStartIterator->get<std::uint64_t>();
                    sample.loopEndFrame = loopEndIterator->get<std::uint64_t>();

                    if (sample.loopStartFrame > sample.loopEndFrame)
                        addIssue(result, context + " loopStartFrame cannot exceed loopEndFrame.");
                }
            }

            if (sample.payloadSizeBytes == 0)
                addIssue(result, context + " must declare payloadSizeBytes greater than zero.");

            if (sample.prefetchBytes > sample.payloadSizeBytes)
                addIssue(result, context + " prefetchBytes cannot exceed payloadSizeBytes.");

            if (container.pageSizeBytes != 0 && sample.payloadOffsetBytes % container.pageSizeBytes != 0)
            {
                addIssue(result,
                         context + " payloadOffsetBytes must be aligned to the declared pageSizeBytes.");
            }

            if (sampleIndex > 0 && sample.payloadOffsetBytes < previousAlignedEnd)
            {
                addIssue(result,
                         context + " payloadOffsetBytes overlaps the previous sample's payload range.");
            }

            const auto pagesIterator = sampleObject.find("pages");
            if (pagesIterator == sampleObject.end() || !isObjectArray(*pagesIterator))
            {
                addIssue(result, context + " field 'pages' must be an array of objects.");
            }
            else
            {
                sample.pages.reserve(pagesIterator->size());

                const auto streamedBytes = sample.payloadSizeBytes >= sample.prefetchBytes
                    ? sample.payloadSizeBytes - sample.prefetchBytes
                    : 0;
                const auto expectedPageCount = container.pageSizeBytes == 0
                    ? 0
                    : static_cast<std::size_t>((streamedBytes + container.pageSizeBytes - 1) / container.pageSizeBytes);

                if (pagesIterator->size() != expectedPageCount)
                {
                    addIssue(result,
                             context + " page-table count did not match the declared payload and prefetch layout.");
                }

                std::uint64_t expectedPageOffset = sample.payloadOffsetBytes + sample.prefetchBytes;
                std::uint64_t remainingStreamedBytes = streamedBytes;

                for (std::size_t pageVectorIndex = 0; pageVectorIndex < pagesIterator->size(); ++pageVectorIndex)
                {
                    const auto& pageObject = pagesIterator->at(pageVectorIndex);
                    const auto pageContext = context + ".Page[" + std::to_string(pageVectorIndex) + "]";
                    RuntimeStreamPageDefinition page;

                    if (const auto pageIndex = readRequired<std::uint32_t>(pageObject, result, "pageIndex", pageContext.c_str()))
                        page.pageIndex = *pageIndex;

                    if (const auto offsetBytes = readRequired<std::uint64_t>(pageObject, result, "offsetBytes", pageContext.c_str()))
                        page.offsetBytes = *offsetBytes;

                    if (const auto sizeBytes = readRequired<std::uint64_t>(pageObject, result, "sizeBytes", pageContext.c_str()))
                        page.sizeBytes = *sizeBytes;

                    if (page.pageIndex != pageVectorIndex)
                        addIssue(result, pageContext + " pageIndex must match its ordered position.");

                    const auto expectedPageSize = std::min(container.pageSizeBytes, remainingStreamedBytes);

                    if (page.offsetBytes != expectedPageOffset)
                        addIssue(result, pageContext + " offsetBytes did not match the expected stream offset.");

                    if (page.sizeBytes != expectedPageSize)
                        addIssue(result, pageContext + " sizeBytes did not match the expected page span.");

                    if (page.sizeBytes == 0)
                        addIssue(result, pageContext + " sizeBytes must be greater than zero.");

                    sample.pages.push_back(std::move(page));
                    ++result.metrics.pageCount;

                    expectedPageOffset += expectedPageSize;
                    remainingStreamedBytes -= expectedPageSize;
                }
            }

            accumulatedPayloadBytes += sample.payloadSizeBytes;
            previousAlignedEnd = alignUp(sample.payloadOffsetBytes + sample.payloadSizeBytes, container.pageSizeBytes);
            container.samples.push_back(std::move(sample));
        }

        if (accumulatedPayloadBytes != container.totalPayloadBytes)
        {
            addIssue(result,
                     "Stream-container totalPayloadBytes did not match the sum of sample payload sizes.");
        }

        if (embeddedPayloadBytes != nullptr)
        {
            result.metrics.payloadAssetResolved = true;
            container.payloadEmbedded = true;
            container.embeddedPayloadBytes = *embeddedPayloadBytes;

            if (container.payloadFileBytes != 0
                && container.embeddedPayloadBytes.size() != container.payloadFileBytes)
            {
                addIssue(result, "Embedded stream payload size did not match payloadFileBytes.");
            }
            else
            {
                container.payloadFileBytes = container.embeddedPayloadBytes.size();
            }

            if (!container.payloadFileChecksumHex.empty())
            {
                const auto actualPayloadChecksum = computeFnv1aChecksumHexForRange(container.embeddedPayloadBytes,
                                                                                   0,
                                                                                   container.embeddedPayloadBytes.size());
                if (actualPayloadChecksum != container.payloadFileChecksumHex)
                {
                    addIssue(result,
                             "Stream payload asset checksum mismatch: expected " + container.payloadFileChecksumHex
                                 + ", actual " + actualPayloadChecksum + ".");
                }
                else
                {
                    ++result.metrics.payloadChecksumValidatedCount;
                }
            }

            for (std::size_t sampleIndex = 0; sampleIndex < container.samples.size(); ++sampleIndex)
            {
                const auto& sample = container.samples[sampleIndex];
                const auto context = "StreamSample[" + std::to_string(sampleIndex) + "]";

                if (!sample.payloadChecksumHex.empty())
                {
                    const auto payloadEndBytes = sample.payloadOffsetBytes + sample.payloadSizeBytes;
                    if (payloadEndBytes > container.payloadFileBytes)
                    {
                        addIssue(result,
                                 context + " payload bytes extend past the declared embedded stream payload.");
                        continue;
                    }

                    const auto actualSampleChecksum = computeFnv1aChecksumHexForRange(container.embeddedPayloadBytes,
                                                                                      sample.payloadOffsetBytes,
                                                                                      sample.payloadSizeBytes);
                    if (actualSampleChecksum.empty())
                    {
                        addIssue(result,
                                 context + " could not compute payload checksum from the embedded stream payload.");
                    }
                    else if (actualSampleChecksum != sample.payloadChecksumHex)
                    {
                        addIssue(result,
                                 context + " payload checksum mismatch: expected " + sample.payloadChecksumHex
                                     + ", actual " + actualSampleChecksum + ".");
                    }
                    else
                    {
                        ++result.metrics.payloadChecksumValidatedCount;
                    }
                }
                else
                {
                    addIssue(result, context + " must provide payloadChecksumHex when stream payload bytes are present.");
                }
            }
        }
        else if (!container.payloadAssetPath.empty())
        {
            std::error_code errorCode;
            const fs::path payloadFsPath(container.payloadAssetPath);
            if (!fs::exists(payloadFsPath, errorCode))
            {
                addIssue(result, "Stream payload asset does not exist: " + container.payloadAssetPath);
            }
            else
            {
                result.metrics.payloadAssetResolved = true;

                const auto payloadFileBytes = fs::file_size(payloadFsPath, errorCode);
                if (errorCode)
                {
                    addIssue(result, "Could not stat stream payload asset: " + container.payloadAssetPath);
                }
                else if (container.payloadFileBytes != 0 && payloadFileBytes != container.payloadFileBytes)
                {
                    addIssue(result, "Stream payload asset size did not match payloadFileBytes.");
                }
                else
                {
                    container.payloadFileBytes = payloadFileBytes;
                }

                if (!container.payloadFileChecksumHex.empty())
                {
                    const auto actualPayloadChecksum = computeFnv1aChecksumHex(payloadFsPath);
                    if (actualPayloadChecksum.empty())
                    {
                        addIssue(result, "Could not compute checksum for stream payload asset: " + container.payloadAssetPath);
                    }
                    else if (actualPayloadChecksum != container.payloadFileChecksumHex)
                    {
                        addIssue(result,
                                 "Stream payload asset checksum mismatch: expected " + container.payloadFileChecksumHex
                                     + ", actual " + actualPayloadChecksum + ".");
                    }
                    else
                    {
                        ++result.metrics.payloadChecksumValidatedCount;
                    }
                }

                for (std::size_t sampleIndex = 0; sampleIndex < container.samples.size(); ++sampleIndex)
                {
                    const auto& sample = container.samples[sampleIndex];
                    const auto context = "StreamSample[" + std::to_string(sampleIndex) + "]";

                    if (!sample.payloadChecksumHex.empty())
                    {
                        const auto payloadEndBytes = sample.payloadOffsetBytes + sample.payloadSizeBytes;
                        if (payloadEndBytes > container.payloadFileBytes)
                        {
                            addIssue(result,
                                     context + " payload bytes extend past the declared stream payload asset.");
                            continue;
                        }

                        const auto actualSampleChecksum = computeFnv1aChecksumHexForRange(payloadFsPath,
                                                                                          sample.payloadOffsetBytes,
                                                                                          sample.payloadSizeBytes);
                        if (actualSampleChecksum.empty())
                        {
                            addIssue(result,
                                     context + " could not compute payload checksum from the stream payload asset.");
                        }
                        else if (actualSampleChecksum != sample.payloadChecksumHex)
                        {
                            addIssue(result,
                                     context + " payload checksum mismatch: expected " + sample.payloadChecksumHex
                                         + ", actual " + actualSampleChecksum + ".");
                        }
                        else
                        {
                            ++result.metrics.payloadChecksumValidatedCount;
                        }
                    }
                    else
                    {
                        addIssue(result, context + " must provide payloadChecksumHex when payloadAssetPath is present.");
                    }
                }
            }
        }
    }

    container.notes = readRequiredStringArray(root, result, "notes", "StreamContainer");

    result.metrics.sampleCount = container.samples.size();

    if (container.schemaName != "drs.streamContainer")
        addIssue(result, "Stream-container schemaName must be 'drs.streamContainer' for the Phase 1 runtime reader.");

    if (container.schemaVersion != 1)
        addIssue(result, "Stream-container schemaVersion must be 1 for the Phase 1 runtime reader.");

    if (container.pageSizeBytes == 0)
        addIssue(result, "Stream-container pageSizeBytes must be greater than zero.");

    if (declaredSampleCount && *declaredSampleCount != container.samples.size())
        addIssue(result, "Stream-container sampleCount did not match the number of parsed sample entries.");

    result.metrics.payloadLayoutValidated = result.issues.empty();
    result.metrics.pageTableValidated = result.issues.empty();
    result.loaded = result.issues.empty();
    result.state = result.loaded ? "Stream-container loaded" : "Stream-container invalid";
    return result;
}

RuntimeStreamLoadResult loadRuntimeStreamContainer(const std::string& containerPath)
{
    RuntimeStreamLoadResult result;
    result.containerPath = containerPath;
    result.state = "Stream-container load not attempted";

    const fs::path containerFsPath(containerPath);
    std::error_code errorCode;

    if (!fs::exists(containerFsPath, errorCode))
    {
        result.state = "Stream-container missing";
        addIssue(result, "Stream-container file was not found at " + containerPath + ".");
        return result;
    }

    const auto rawText = readTextFile(containerFsPath);
    if (rawText.empty())
    {
        result.containerFound = true;
        result.state = "Stream-container unreadable";
        addIssue(result, "Stream-container file was empty or unreadable.");
        return result;
    }

    return parseRuntimeStreamContainer(rawText, containerPath, true, nullptr);
}

RuntimeStreamLoadResult loadPhase1ReferenceStreamContainer()
{
    return loadRuntimeStreamContainer(getPhase1ReferenceStreamContainerPath());
}

RuntimeStreamLoadResult loadRuntimeStreamContainerForInstrument(const RuntimeManifestLoadResult& instrumentResult)
{
    if (!instrumentResult.manifestFound || instrumentResult.instrument.compiledStreamAssetPath.empty())
    {
        RuntimeStreamLoadResult result;
        result.state = "Stream-container path unavailable";
        result.containerPath = instrumentResult.instrument.compiledStreamAssetPath;
        result.containerFound = false;
        addIssue(result, "Instrument result did not provide a resolved compiled stream asset path.");
        return result;
    }

    return loadRuntimeStreamContainer(instrumentResult.instrument.compiledStreamAssetPath);
}

RuntimeStreamReadResult resolveRuntimeStreamRead(const RuntimeStreamContainerModel& container,
                                                 const std::string& sampleId,
                                                 std::uint64_t payloadRelativeOffsetBytes)
{
    RuntimeStreamReadResult result;
    result.sampleId = sampleId;
    result.payloadRelativeOffsetBytes = payloadRelativeOffsetBytes;
    result.state = "Stream read not resolved";

    const auto sampleIterator = std::find_if(container.samples.begin(),
                                             container.samples.end(),
                                             [&](const RuntimeStreamSampleDefinition& sample)
                                             {
                                                 return sample.sampleId == sampleId;
                                             });

    if (sampleIterator == container.samples.end())
    {
        result.state = "Stream sample missing";
        return result;
    }

    const auto& sample = *sampleIterator;
    if (payloadRelativeOffsetBytes >= sample.payloadSizeBytes)
    {
        result.state = "Stream offset out of range";
        return result;
    }

    result.resolved = true;
    if (payloadRelativeOffsetBytes < sample.prefetchBytes)
    {
        result.inPrefetchHead = true;
        result.absoluteOffsetBytes = sample.payloadOffsetBytes + payloadRelativeOffsetBytes;
        result.readableBytes = sample.prefetchBytes - payloadRelativeOffsetBytes;
        result.state = "Resolved in prefetch head";
        return result;
    }

    for (const auto& page : sample.pages)
    {
        const auto pageRelativeOffset = page.offsetBytes - sample.payloadOffsetBytes;
        if (payloadRelativeOffsetBytes >= pageRelativeOffset
            && payloadRelativeOffsetBytes < pageRelativeOffset + page.sizeBytes)
        {
            result.inPageTable = true;
            result.pageIndex = page.pageIndex;
            result.absoluteOffsetBytes = sample.payloadOffsetBytes + payloadRelativeOffsetBytes;
            result.readableBytes = (pageRelativeOffset + page.sizeBytes) - payloadRelativeOffsetBytes;
            result.state = "Resolved in page table";
            return result;
        }
    }

    result.resolved = false;
    result.state = "Stream page missing";
    return result;
}
} // namespace drs::engine
