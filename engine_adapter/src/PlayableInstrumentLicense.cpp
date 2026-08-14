#include "drs/engine/PlayableInstrumentLicense.h"

namespace drs::engine
{
PlayableInstrumentLicenseValidationResult validatePlayableInstrumentLicenseBytes(
    const std::vector<std::uint8_t>& bytes)
{
    PlayableInstrumentLicenseValidationResult result;
    if (bytes.size() > maximumPlayableInstrumentLicenseBytes)
    {
        result.issue = "License text exceeds the 1 MiB limit.";
        return result;
    }

    for (std::size_t index = 0; index < bytes.size();)
    {
        const auto lead = bytes[index];
        if (lead == 0)
        {
            result.issue = "License text contains an embedded NUL byte.";
            return result;
        }

        if (lead < 0x80u)
        {
            if (lead == 0x7fu
                || (lead < 0x20u && lead != '\t' && lead != '\r' && lead != '\n'))
            {
                result.issue = "License text contains binary control bytes.";
                return result;
            }
            ++index;
            continue;
        }

        std::size_t continuationCount = 0;
        if (lead >= 0xc2u && lead <= 0xdfu)
            continuationCount = 1;
        else if (lead >= 0xe0u && lead <= 0xefu)
            continuationCount = 2;
        else if (lead >= 0xf0u && lead <= 0xf4u)
            continuationCount = 3;
        else
        {
            result.issue = "License text is not valid UTF-8.";
            return result;
        }

        if (continuationCount > bytes.size() - index - 1)
        {
            result.issue = "License text is not valid UTF-8.";
            return result;
        }

        const auto second = bytes[index + 1];
        if ((second & 0xc0u) != 0x80u
            || (lead == 0xe0u && second < 0xa0u)
            || (lead == 0xedu && second > 0x9fu)
            || (lead == 0xf0u && second < 0x90u)
            || (lead == 0xf4u && second > 0x8fu))
        {
            result.issue = "License text is not valid UTF-8.";
            return result;
        }

        for (std::size_t offset = 2; offset <= continuationCount; ++offset)
        {
            if ((bytes[index + offset] & 0xc0u) != 0x80u)
            {
                result.issue = "License text is not valid UTF-8.";
                return result;
            }
        }
        index += continuationCount + 1;
    }

    result.valid = true;
    return result;
}
} // namespace drs::engine
