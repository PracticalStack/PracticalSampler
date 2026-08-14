#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
inline constexpr const char* playableInstrumentLicenseFileName = "LICENSE.txt";
inline constexpr const char* playableInstrumentLicensePayloadId = "license-text";
inline constexpr const char* playableInstrumentLicenseLogicalPath = "LICENSE.txt";
inline constexpr const char* playableInstrumentLicenseMediaType = "text/plain; charset=utf-8";
inline constexpr std::uint64_t maximumPlayableInstrumentLicenseBytes = 1024ull * 1024ull;
inline constexpr std::uint32_t playableInstrumentLicensePackageV2RecordKind = 7;

inline constexpr bool playableInstrumentLicenseRequiresUtf8 = true;
inline constexpr bool playableInstrumentLicenseAllowsUtf8Bom = true;
inline constexpr bool playableInstrumentLicenseAllowsEmbeddedNull = false;
inline constexpr bool playableInstrumentLicensePreservesExactBytes = true;
inline constexpr bool playableInstrumentLicenseRequiresPackageSchemaBump = false;

struct PlayableInstrumentLicenseValidationResult
{
    bool valid = false;
    std::string issue;
};

PlayableInstrumentLicenseValidationResult validatePlayableInstrumentLicenseBytes(
    const std::vector<std::uint8_t>& bytes);
} // namespace drs::engine
