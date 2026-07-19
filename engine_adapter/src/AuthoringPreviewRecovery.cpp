#include "drs/engine/AuthoringPreviewRecovery.h"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <utility>

namespace drs::engine
{
namespace
{
std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
    {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

bool containsAny(const std::string& value,
                 std::initializer_list<const char*> fragments)
{
    return std::any_of(fragments.begin(), fragments.end(), [&](const char* fragment)
    {
        return value.find(fragment) != std::string::npos;
    });
}
} // namespace

AuthoringPreviewFailureFinding classifyAuthoringPreviewFailure(
    std::string code,
    std::string path,
    std::string message)
{
    AuthoringPreviewFailureFinding finding;
    finding.code = code.empty() ? "preview-preparation-failed" : std::move(code);
    finding.path = std::move(path);
    finding.message = message.empty() ? "Authoring Preview preparation failed." : std::move(message);

    const auto searchable = lower(finding.code + " " + finding.path + " " + finding.message);
    if (containsAny(searchable, { "cancel", "supersed" }))
        finding.family = AuthoringPreviewFailureFamily::cancellation;
    else if (containsAny(searchable, { "slot", "exhaust", "resource-pressure", "capacity" }))
        finding.family = AuthoringPreviewFailureFamily::resourcePressure;
    else if (containsAny(searchable, { "unsupported", "format", "channel-count", "mono or stereo" }))
        finding.family = AuthoringPreviewFailureFamily::unsupportedFormat;
    else if (containsAny(searchable, { "decode", "checksum", "truncated" }))
        finding.family = AuthoringPreviewFailureFamily::decodeFailure;
    else if (containsAny(searchable, { "missing", "not found", "could not open", "unknown-zone-sample-source" }))
        finding.family = AuthoringPreviewFailureFamily::missingSource;
    else if (containsAny(searchable, { "loop", "range", "root-key", "start-frame", "velocity" }))
        finding.family = AuthoringPreviewFailureFamily::invalidRange;
    else if (containsAny(searchable, { "route", "topology", "duplicate", "conflict", "articulation", "group" }))
        finding.family = AuthoringPreviewFailureFamily::routeConflict;
    else
        finding.family = AuthoringPreviewFailureFamily::internal;

    finding.retryable = finding.family != AuthoringPreviewFailureFamily::cancellation;
    return finding;
}

const char* toString(AuthoringPreviewFailureFamily family) noexcept
{
    switch (family)
    {
        case AuthoringPreviewFailureFamily::none: return "none";
        case AuthoringPreviewFailureFamily::missingSource: return "missing-source";
        case AuthoringPreviewFailureFamily::unsupportedFormat: return "unsupported-format";
        case AuthoringPreviewFailureFamily::invalidRange: return "invalid-range";
        case AuthoringPreviewFailureFamily::routeConflict: return "route-conflict";
        case AuthoringPreviewFailureFamily::decodeFailure: return "decode-failure";
        case AuthoringPreviewFailureFamily::cancellation: return "cancellation";
        case AuthoringPreviewFailureFamily::resourcePressure: return "resource-pressure";
        case AuthoringPreviewFailureFamily::internal: return "internal";
    }
    return "internal";
}

std::string formatAuthoringPreviewFailure(const AuthoringPreviewFailureFinding& finding)
{
    auto text = "[" + finding.code + "] " + finding.message;
    if (!finding.path.empty())
        text += " (" + finding.path + ")";
    return text;
}
} // namespace drs::engine

