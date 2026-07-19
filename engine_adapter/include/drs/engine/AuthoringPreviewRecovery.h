#pragma once

#include <cstdint>
#include <string>

namespace drs::engine
{
enum class AuthoringPreviewFailureFamily : std::uint8_t
{
    none = 0,
    missingSource,
    unsupportedFormat,
    invalidRange,
    routeConflict,
    decodeFailure,
    cancellation,
    resourcePressure,
    internal
};

struct AuthoringPreviewFailureFinding
{
    AuthoringPreviewFailureFamily family = AuthoringPreviewFailureFamily::none;
    std::string code;
    std::string path;
    std::string message;
    bool retryable = false;
};

AuthoringPreviewFailureFinding classifyAuthoringPreviewFailure(
    std::string code,
    std::string path,
    std::string message);
const char* toString(AuthoringPreviewFailureFamily family) noexcept;
std::string formatAuthoringPreviewFailure(const AuthoringPreviewFailureFinding& finding);
} // namespace drs::engine

