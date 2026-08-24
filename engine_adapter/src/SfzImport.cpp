#include "drs/engine/SfzImport.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <unordered_map>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;

void addFinding(std::vector<SfzImportFinding>& findings,
                SfzImportFindingSeverity severity,
                SfzImportSupportDisposition disposition,
                const std::string& code,
                const std::string& summary,
                const std::string& detail,
                const SfzImportSourceLocation& location = {},
                std::size_t maximumFindingCount = std::numeric_limits<std::size_t>::max(),
                std::size_t* suppressedFindingCount = nullptr)
{
    if (findings.size() >= maximumFindingCount)
    {
        if (suppressedFindingCount != nullptr)
            ++(*suppressedFindingCount);
        return;
    }

    SfzImportFinding finding;
    finding.severity = severity;
    finding.disposition = disposition;
    finding.code = code;
    finding.summary = summary;
    finding.detail = detail;
    finding.location = location;
    findings.push_back(std::move(finding));
}

std::string toLowerAscii(const std::string& text)
{
    std::string lowered = text;
    std::transform(lowered.begin(),
                   lowered.end(),
                   lowered.begin(),
                   [](unsigned char character)
                   {
                       return static_cast<char>(std::tolower(character));
                   });
    return lowered;
}

std::string trimAscii(const std::string& text)
{
    auto begin = text.begin();
    while (begin != text.end() && std::isspace(static_cast<unsigned char>(*begin)) != 0)
        ++begin;

    auto end = text.end();
    while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1))) != 0)
        --end;

    return std::string(begin, end);
}

std::string toDisplayPath(const fs::path& path)
{
    return path.lexically_normal().generic_string();
}

std::string stripLineComment(const std::string& line)
{
    auto inQuotes = false;

    for (std::size_t index = 0; index < line.size(); ++index)
    {
        const auto character = line[index];
        if (character == '"' && (index == 0 || line[index - 1] != '\\'))
            inQuotes = !inQuotes;

        if (!inQuotes && character == '/' && index + 1 < line.size() && line[index + 1] == '/')
            return line.substr(0, index);
    }

    return line;
}

SfzOpcodeScope scopeFromHeaderName(const std::string& headerName)
{
    const auto lowered = toLowerAscii(trimAscii(headerName));

    if (lowered == "control")
        return SfzOpcodeScope::control;
    if (lowered == "global")
        return SfzOpcodeScope::global;
    if (lowered == "master")
        return SfzOpcodeScope::master;
    if (lowered == "group")
        return SfzOpcodeScope::group;
    if (lowered == "region")
        return SfzOpcodeScope::region;
    if (lowered == "curve")
        return SfzOpcodeScope::curve;
    if (lowered == "effect")
        return SfzOpcodeScope::effect;
    if (lowered == "midi")
        return SfzOpcodeScope::midi;
    if (lowered == "sample")
        return SfzOpcodeScope::sample;

    return SfzOpcodeScope::unknown;
}

void addSourceFile(SfzParsedDocument& document, const fs::path& path)
{
    const auto displayPath = toDisplayPath(path);
    if (std::find(document.sourceFiles.begin(), document.sourceFiles.end(), displayPath)
        == document.sourceFiles.end())
    {
        document.sourceFiles.push_back(displayPath);
    }
}

bool parseIncludeDirective(const std::string& line, std::string& includePath)
{
    const auto trimmed = trimAscii(line);
    if (trimmed.rfind("#include", 0) != 0
        || (trimmed.size() > 8
            && std::isspace(static_cast<unsigned char>(trimmed[8])) == 0))
        return false;

    auto remainder = trimAscii(trimmed.substr(8));
    if (remainder.empty())
        return false;

    if (!remainder.empty() && remainder.front() == '"' && remainder.back() == '"' && remainder.size() >= 2)
    {
        includePath = remainder.substr(1, remainder.size() - 2);
        return true;
    }

    includePath = remainder;
    return true;
}

using SfzMacroMap = std::unordered_map<std::string, std::string>;

bool isMacroNameCharacter(const char character) noexcept
{
    return std::isalnum(static_cast<unsigned char>(character)) != 0 || character == '_';
}

std::size_t findNextDefineDirective(const std::string& text,
                                    const std::size_t searchStart) noexcept
{
    for (std::size_t index = searchStart; index + 7 <= text.size(); ++index)
    {
        if (text.compare(index, 7, "#define") != 0)
            continue;
        const auto hasLeadingBoundary = index == 0
            || std::isspace(static_cast<unsigned char>(text[index - 1])) != 0;
        const auto hasTrailingBoundary = index + 7 == text.size()
            || std::isspace(static_cast<unsigned char>(text[index + 7])) != 0;
        if (hasLeadingBoundary && hasTrailingBoundary)
            return index;
    }
    return std::string::npos;
}

bool parseDefineDirective(const std::string& line,
                          std::string& macroName,
                          std::string& macroValue)
{
    const auto trimmed = trimAscii(line);
    if (trimmed.rfind("#define", 0) != 0
        || (trimmed.size() > 7
            && std::isspace(static_cast<unsigned char>(trimmed[7])) == 0))
        return false;

    auto remainder = trimAscii(trimmed.substr(7));
    if (remainder.empty() || remainder.front() != '$')
        return true;

    std::size_t nameEnd = 1;
    while (nameEnd < remainder.size() && isMacroNameCharacter(remainder[nameEnd]))
        ++nameEnd;

    if (nameEnd == 1)
        return true;

    macroName = remainder.substr(0, nameEnd);
    const auto nextDefine = findNextDefineDirective(remainder, nameEnd);
    macroValue = trimAscii(remainder.substr(nameEnd,
                                             nextDefine == std::string::npos
                                                 ? std::string::npos
                                                 : nextDefine - nameEnd));
    return true;
}

bool isOpcodeNameCharacter(const char character) noexcept
{
    return std::isalnum(static_cast<unsigned char>(character)) != 0
        || character == '_'
        || character == '$';
}

std::size_t findNextOpcodeOrHeader(const std::string& line, std::size_t searchStart)
{
    for (auto index = searchStart; index < line.size(); ++index)
    {
        if (std::isspace(static_cast<unsigned char>(line[index])) == 0)
            continue;

        auto candidate = index;
        while (candidate < line.size()
               && std::isspace(static_cast<unsigned char>(line[candidate])) != 0)
        {
            ++candidate;
        }

        if (candidate >= line.size())
            return line.size();
        if (line[candidate] == '<')
            return candidate;
        if (std::isalpha(static_cast<unsigned char>(line[candidate])) == 0
            && line[candidate] != '_')
        {
            continue;
        }

        auto nameEnd = candidate;
        while (nameEnd < line.size() && isOpcodeNameCharacter(line[nameEnd]))
            ++nameEnd;
        auto assignment = nameEnd;
        while (assignment < line.size()
               && std::isspace(static_cast<unsigned char>(line[assignment])) != 0)
        {
            ++assignment;
        }
        if (assignment < line.size() && line[assignment] == '=')
            return candidate;
    }

    return line.size();
}

using ResolvedOpcodeMap = std::map<std::string, SfzResolvedOpcode>;

ResolvedOpcodeMap buildLocalOpcodeMap(const SfzParsedSection& section,
                                      const SfzImportExecutionContext& context,
                                      bool& canceled)
{
    ResolvedOpcodeMap map;
    for (const auto& opcode : section.opcodes)
    {
        if (context.isCancellationRequested())
        {
            canceled = true;
            break;
        }
        map[opcode.name] = { opcode.name, opcode.value, opcode.location, false, opcode.resolutionBasePath };
    }
    return map;
}

void overlayOpcodes(ResolvedOpcodeMap& destination,
                    const ResolvedOpcodeMap& source,
                    bool inherited,
                    const SfzImportExecutionContext& context,
                    bool& canceled)
{
    for (const auto& [name, opcode] : source)
    {
        if (context.isCancellationRequested())
        {
            canceled = true;
            break;
        }
        auto resolved = opcode;
        resolved.inherited = inherited;
        destination[name] = std::move(resolved);
    }
}

std::vector<SfzResolvedOpcode> toResolvedOpcodeVector(const ResolvedOpcodeMap& map)
{
    std::vector<SfzResolvedOpcode> result;
    result.reserve(map.size());
    for (const auto& [_, opcode] : map)
        result.push_back(opcode);
    return result;
}

struct SfzParserState
{
    SfzParsedDocument& document;
    std::vector<SfzImportFinding>& findings;
    std::vector<fs::path> includeStack;
    SfzMacroMap macros;
    fs::path rootResolutionBasePath;
    std::size_t currentSectionIndex = static_cast<std::size_t>(-1);
    std::size_t nextDocumentOrder = 0;
    std::size_t totalSourceBytes = 0;
    std::size_t includeCount = 0;
    std::size_t regionCount = 0;
    std::size_t suppressedFindingCount = 0;
    bool hadErrorFinding = false;
    bool budgetExceeded = false;
};

void addParserFinding(SfzParserState& state,
                      const SfzImportExecutionContext& context,
                      SfzImportFindingSeverity severity,
                      SfzImportSupportDisposition disposition,
                      const std::string& code,
                      const std::string& summary,
                      const std::string& detail,
                      const SfzImportSourceLocation& location = {})
{
    state.hadErrorFinding = state.hadErrorFinding || severity == SfzImportFindingSeverity::error;
    addFinding(state.findings,
               severity,
               disposition,
               code,
               summary,
               detail,
               location,
               context.budgets.maximumFindingCount,
               &state.suppressedFindingCount);
}

std::string expandMacros(const std::string& line,
                         SfzParserState& state,
                         const SfzImportExecutionContext& context,
                         const fs::path& sourcePath,
                         const std::size_t lineNumber)
{
    std::string expanded;
    expanded.reserve(line.size());

    for (std::size_t index = 0; index < line.size();)
    {
        if (line[index] != '$')
        {
            expanded.push_back(line[index++]);
            continue;
        }

        auto nameEnd = index + 1;
        while (nameEnd < line.size() && isMacroNameCharacter(line[nameEnd]))
            ++nameEnd;
        auto macroName = line.substr(index, nameEnd - index);
        auto macro = state.macros.find(macroName);
        // A number of real-world SFZ libraries concatenate a macro with a
        // literal suffix (for example "$LAB_TOP1") without a delimiter.  If
        // the greedy token is unknown, fall back to the longest known prefix
        // and leave the suffix in the input stream.
        if (macro == state.macros.end())
        {
            for (auto candidateEnd = nameEnd; candidateEnd > index + 1; --candidateEnd)
            {
                const auto candidate = line.substr(index, candidateEnd - index);
                const auto candidateIterator = state.macros.find(candidate);
                if (candidateIterator != state.macros.end())
                {
                    macroName = candidate;
                    nameEnd = candidateEnd;
                    macro = candidateIterator;
                    break;
                }
            }
        }
        if (macro == state.macros.end())
        {
            addParserFinding(state,
                             context,
                             SfzImportFindingSeverity::error,
                             SfzImportSupportDisposition::blocking,
                             "preprocessor.macro_undefined",
                             "Undefined SFZ macro",
                             "Macro '" + macroName + "' was used before it was defined.",
                             { toDisplayPath(sourcePath), lineNumber, index + 1,
                               SfzOpcodeScope::unknown, macroName });
            expanded += macroName;
        }
        else
        {
            expanded += macro->second;
        }
        index = nameEnd;
    }

    return expanded;
}

bool parseFileRecursive(const fs::path& filePath,
                        SfzParserState& state,
                        const SfzImportExecutionContext& context,
                        SfzImportCancellationReason& cancellationReason)
{
    cancellationReason = context.pollCancellation();
    if (cancellationReason != SfzImportCancellationReason::none)
        return false;

    const auto normalizedPath = filePath.lexically_normal();
    if (!state.includeStack.empty()
        && state.includeStack.size() >= context.budgets.maximumIncludeDepth)
    {
        state.budgetExceeded = true;
        addParserFinding(state, context,
                         SfzImportFindingSeverity::error,
                         SfzImportSupportDisposition::blocking,
                         "budget.include_depth_exceeded",
                         "SFZ include-depth budget exceeded",
                         "The document exceeded the maximum include depth of "
                             + std::to_string(context.budgets.maximumIncludeDepth) + ".",
                         { toDisplayPath(normalizedPath), 0, 0, SfzOpcodeScope::unknown, "#include" });
        return false;
    }

    const auto isIncludedFile = !state.includeStack.empty();
    if (isIncludedFile && state.includeCount >= context.budgets.maximumIncludeCount)
    {
        state.budgetExceeded = true;
        addParserFinding(state, context,
                         SfzImportFindingSeverity::error,
                         SfzImportSupportDisposition::blocking,
                         "budget.include_count_exceeded",
                         "SFZ include budget exceeded",
                         "The document exceeded the maximum include count of "
                             + std::to_string(context.budgets.maximumIncludeCount) + ".",
                         { toDisplayPath(normalizedPath), 0, 0, SfzOpcodeScope::unknown, "#include" });
        return false;
    }

    const auto cycleIterator = std::find(state.includeStack.begin(), state.includeStack.end(), normalizedPath);
    if (cycleIterator != state.includeStack.end())
    {
        addParserFinding(state, context,
                         SfzImportFindingSeverity::error,
                         SfzImportSupportDisposition::blocking,
                         "include.cycle",
                         "Include cycle detected",
                         "SFZ include recursion re-entered '" + toDisplayPath(normalizedPath) + "'.",
                         { toDisplayPath(normalizedPath), 0, 0, SfzOpcodeScope::unknown, "#include" });
        return false;
    }

    std::error_code sizeError;
    const auto fileBytes = fs::file_size(normalizedPath, sizeError);
    if (sizeError)
    {
        addParserFinding(state, context,
                         SfzImportFindingSeverity::error,
                         SfzImportSupportDisposition::blocking,
                         "source.missing",
                         "SFZ source file missing",
                         "Could not inspect SFZ source file '" + toDisplayPath(normalizedPath) + "'.",
                         { toDisplayPath(normalizedPath), 0, 0, SfzOpcodeScope::unknown, "" });
        return false;
    }

    if (fileBytes > context.budgets.maximumTotalSourceBytes
        || state.totalSourceBytes > context.budgets.maximumTotalSourceBytes - static_cast<std::size_t>(fileBytes))
    {
        state.budgetExceeded = true;
        addParserFinding(state, context,
                         SfzImportFindingSeverity::error,
                         SfzImportSupportDisposition::blocking,
                         "budget.source_bytes_exceeded",
                         "SFZ source-byte budget exceeded",
                         "The expanded document exceeded the maximum source budget of "
                             + std::to_string(context.budgets.maximumTotalSourceBytes) + " bytes.",
                         { toDisplayPath(normalizedPath), 0, 0, SfzOpcodeScope::unknown, "" });
        return false;
    }

    std::ifstream input(normalizedPath, std::ios::binary);
    if (!input.good())
    {
        addParserFinding(state, context,
                         SfzImportFindingSeverity::error,
                         SfzImportSupportDisposition::blocking,
                         "source.missing",
                         "SFZ source file missing",
                         "Could not open SFZ source file '" + toDisplayPath(normalizedPath) + "'.",
                         { toDisplayPath(normalizedPath), 0, 0, SfzOpcodeScope::unknown, "" });
        return false;
    }

    state.totalSourceBytes += static_cast<std::size_t>(fileBytes);
    if (isIncludedFile)
        ++state.includeCount;
    addSourceFile(state.document, normalizedPath);
    state.includeStack.push_back(normalizedPath);

    auto complete = true;
    std::string rawLine;
    std::size_t lineNumber = 0;

    while (std::getline(input, rawLine))
    {
        cancellationReason = context.pollCancellation();
        if (cancellationReason != SfzImportCancellationReason::none)
            break;

        ++lineNumber;
        if (!rawLine.empty() && rawLine.back() == '\r')
            rawLine.pop_back();

        const auto sourceLine = stripLineComment(rawLine);
        const auto trimmed = trimAscii(sourceLine);
        if (trimmed.empty())
            continue;

        std::string macroName;
        std::string macroValue;
        if (parseDefineDirective(sourceLine, macroName, macroValue))
        {
            auto defineLine = trimAscii(sourceLine);
            bool processedDefine = false;
            while (!defineLine.empty() && parseDefineDirective(defineLine, macroName, macroValue))
            {
                processedDefine = true;
                if (macroName.empty() || macroValue.empty())
                {
                    addParserFinding(state, context,
                                     SfzImportFindingSeverity::error,
                                     SfzImportSupportDisposition::blocking,
                                     "preprocessor.define_invalid",
                                     "Invalid SFZ macro definition",
                                     "A #define directive must provide a $name and value.",
                                     { toDisplayPath(normalizedPath), lineNumber, 1,
                                       SfzOpcodeScope::unknown, "#define" });
                }
                else
                {
                    state.macros[macroName] = expandMacros(macroValue, state, context, normalizedPath, lineNumber);
                }

                const auto nextDefine = findNextDefineDirective(defineLine, 7);
                if (nextDefine == std::string::npos)
                    break;
                defineLine = trimAscii(defineLine.substr(nextDefine));
            }
            if (processedDefine)
                continue;
        }

        const auto line = expandMacros(sourceLine, state, context, normalizedPath, lineNumber);

        std::string includePath;
        if (parseIncludeDirective(line, includePath))
        {
            cancellationReason = context.pollCancellation();
            if (cancellationReason != SfzImportCancellationReason::none)
                break;

            auto resolvedIncludePath = (normalizedPath.parent_path() / fs::path(includePath)).lexically_normal();
            // Some SFZ authors write paths such as ../Data/group/foo.txt in
            // files already under Data/stereo or Data/group.  ARIA resolves
            // these against the instrument root; preserve the conventional
            // file-relative attempt first, then use the root-relative form
            // when the first candidate is absent.
            if (!fs::exists(resolvedIncludePath))
            {
                const auto rootRelativeIncludePath =
                    (state.rootResolutionBasePath / fs::path(includePath)).lexically_normal();
                if (fs::exists(rootRelativeIncludePath))
                    resolvedIncludePath = rootRelativeIncludePath;
            }
            const auto includeComplete = parseFileRecursive(resolvedIncludePath,
                                                            state,
                                                            context,
                                                            cancellationReason);
            if (cancellationReason != SfzImportCancellationReason::none)
                break;
            complete = includeComplete && complete;
            if (state.budgetExceeded)
                break;
            continue;
        }

        std::size_t index = 0;
        while (index < line.size())
        {
            while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) != 0)
                ++index;

            if (index >= line.size())
                break;

            if (line[index] == '<')
            {
                const auto headerEnd = line.find('>', index + 1);
                if (headerEnd == std::string::npos)
                {
                    addParserFinding(state, context,
                                     SfzImportFindingSeverity::error,
                                     SfzImportSupportDisposition::blocking,
                                     "syntax.header_unterminated",
                                     "SFZ header was not closed",
                                     "Encountered an unterminated SFZ header.",
                                     { toDisplayPath(normalizedPath), lineNumber, index + 1,
                                       SfzOpcodeScope::unknown, "" });
                    complete = false;
                    break;
                }

                auto headerName = trimAscii(line.substr(index + 1, headerEnd - index - 1));
                headerName = toLowerAscii(headerName);

                const auto scope = scopeFromHeaderName(headerName);
                if (state.document.sections.size() >= context.budgets.maximumSectionCount
                    || (scope == SfzOpcodeScope::region
                        && state.regionCount >= context.budgets.maximumRegionCount))
                {
                    state.budgetExceeded = true;
                    const auto regionLimit = scope == SfzOpcodeScope::region
                        && state.regionCount >= context.budgets.maximumRegionCount;
                    addParserFinding(state, context,
                                     SfzImportFindingSeverity::error,
                                     SfzImportSupportDisposition::blocking,
                                     regionLimit ? "budget.region_count_exceeded"
                                                 : "budget.section_count_exceeded",
                                     regionLimit ? "SFZ region budget exceeded"
                                                 : "SFZ section budget exceeded",
                                     "The document exceeded the configured maximum "
                                         + std::string(regionLimit ? "region" : "section") + " count.",
                                     { toDisplayPath(normalizedPath), lineNumber, index + 1,
                                       scope, headerName });
                    complete = false;
                    break;
                }

                SfzParsedSection section;
                section.scope = scope;
                section.headerName = headerName;
                section.headerLocation = { toDisplayPath(normalizedPath), lineNumber, index + 1, section.scope, headerName };
                section.documentOrder = state.nextDocumentOrder++;
                state.document.sections.push_back(std::move(section));
                state.currentSectionIndex = state.document.sections.size() - 1;
                if (scope == SfzOpcodeScope::region)
                    ++state.regionCount;

                if (state.document.sections.back().scope == SfzOpcodeScope::unknown)
                {
                    addParserFinding(state, context,
                                     SfzImportFindingSeverity::warning,
                                     SfzImportSupportDisposition::reportedOnly,
                                     "header.unknown",
                                     "Unknown SFZ header",
                                     "The parser preserved unknown header '" + headerName
                                         + "' for later compatibility reporting.",
                                     state.document.sections.back().headerLocation);
                }

                index = headerEnd + 1;
                continue;
            }

            const auto keyStart = index;
            while (index < line.size()
                   && std::isspace(static_cast<unsigned char>(line[index])) == 0
                   && line[index] != '=')
            {
                ++index;
            }

            const auto keyText = line.substr(keyStart, index - keyStart);
            auto opcodeName = toLowerAscii(trimAscii(keyText));

            while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) != 0)
                ++index;

            if (opcodeName.empty() || index >= line.size() || line[index] != '=')
            {
                addParserFinding(state, context,
                                 SfzImportFindingSeverity::error,
                                 SfzImportSupportDisposition::blocking,
                                 "syntax.invalid_token",
                                 "Invalid SFZ token",
                                 "Encountered token '" + trimAscii(keyText)
                                     + "' that was not a valid opcode assignment.",
                                 { toDisplayPath(normalizedPath), lineNumber, keyStart + 1,
                                   SfzOpcodeScope::unknown, trimAscii(keyText) });
                complete = false;

                while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) == 0)
                    ++index;
                continue;
            }

            ++index;
            while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) != 0)
                ++index;

            if (state.currentSectionIndex >= state.document.sections.size())
            {
                addParserFinding(state, context,
                                 SfzImportFindingSeverity::error,
                                 SfzImportSupportDisposition::blocking,
                                 "syntax.opcode_without_header",
                                 "Opcode appeared before any SFZ header",
                                 "Encountered opcode '" + opcodeName
                                     + "' before a valid SFZ header had been declared.",
                                 { toDisplayPath(normalizedPath), lineNumber, keyStart + 1,
                                   SfzOpcodeScope::unknown, opcodeName });
                complete = false;

                while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) == 0)
                    ++index;
                continue;
            }

            std::string opcodeValue;
            if (index < line.size() && line[index] == '"')
            {
                const auto valueStart = index + 1;
                ++index;
                auto closed = false;
                while (index < line.size())
                {
                    if (line[index] == '"' && line[index - 1] != '\\')
                    {
                        closed = true;
                        break;
                    }
                    ++index;
                }

                if (!closed)
                {
                    addParserFinding(state, context,
                                     SfzImportFindingSeverity::error,
                                     SfzImportSupportDisposition::blocking,
                                     "syntax.quoted_value_unterminated",
                                     "Quoted SFZ value was not closed",
                                     "Encountered an unterminated quoted value for opcode '" + opcodeName + "'.",
                                     { toDisplayPath(normalizedPath),
                                       lineNumber,
                                       valueStart,
                                       state.document.sections[state.currentSectionIndex].scope,
                                       opcodeName });
                    complete = false;
                    opcodeValue = line.substr(valueStart);
                    index = line.size();
                }
                else
                {
                    opcodeValue = line.substr(valueStart, index - valueStart);
                    ++index;
                }
            }
            else
            {
                const auto valueStart = index;
                index = findNextOpcodeOrHeader(line, valueStart);
                opcodeValue = trimAscii(line.substr(valueStart, index - valueStart));
            }

            if (opcodeValue.empty())
            {
                addParserFinding(state, context,
                                 SfzImportFindingSeverity::error,
                                 SfzImportSupportDisposition::blocking,
                                 "syntax.empty_value",
                                 "SFZ opcode value was empty",
                                 "Encountered opcode '" + opcodeName + "' without a value.",
                                 { toDisplayPath(normalizedPath),
                                   lineNumber,
                                   keyStart + 1,
                                   state.document.sections[state.currentSectionIndex].scope,
                                   opcodeName });
                complete = false;
                continue;
            }

            state.document.sections[state.currentSectionIndex].opcodes.push_back(
                { opcodeName,
                  opcodeValue,
                  { toDisplayPath(normalizedPath),
                    lineNumber,
                    keyStart + 1,
                    state.document.sections[state.currentSectionIndex].scope,
                    opcodeName },
                  toDisplayPath(state.rootResolutionBasePath) });
        }

        if (state.budgetExceeded)
            break;
    }

    state.includeStack.pop_back();
    return complete;
}
} // namespace

SfzDocumentParseResult parseSfzDocument(const std::string& sfzPath)
{
    return parseSfzDocument(sfzPath, defaultSfzImportExecutionContext());
}

SfzDocumentParseResult parseSfzDocument(const std::string& sfzPath,
                                        const SfzImportExecutionContext& context)
{
    SfzDocumentParseResult result;
    context.reportProgress(SfzImportStage::discovering, 0.0f);
    result.document.rootDocumentPath = toDisplayPath(fs::path(sfzPath));
    result.state = "Discovering";

    SfzParserState parserState {
        result.document,
        result.findings,
        {},
        {},
        fs::path(sfzPath).lexically_normal().parent_path()
    };
    SfzImportCancellationReason cancellationReason = SfzImportCancellationReason::none;

    const auto complete = parseFileRecursive(fs::path(sfzPath),
                                             parserState,
                                             context,
                                             cancellationReason);
    result.suppressedFindingCount = parserState.suppressedFindingCount;

    if (cancellationReason != SfzImportCancellationReason::none)
    {
        result.execution.disposition = SfzImportExecutionDisposition::canceled;
        result.execution.cancellationReason = cancellationReason;
        result.state = "Canceled";
        context.reportProgress(SfzImportStage::canceled, 0.30f);
        return result;
    }

    result.parsed = !parserState.hadErrorFinding;
    result.complete = complete && result.parsed;

    if (!result.parsed)
    {
        result.state = "Blocked";
        result.execution.disposition = SfzImportExecutionDisposition::failed;
        result.execution.failureReason = parserState.budgetExceeded
            ? SfzImportFailureReason::budgetExceeded
            : std::any_of(result.findings.begin(),
                                                     result.findings.end(),
                                                     [](const auto& finding)
                                                     {
                                                         return finding.code == "source.missing";
                                                     })
            ? SfzImportFailureReason::sourceMissing
            : SfzImportFailureReason::malformedInput;
    }
    else if (!result.complete)
    {
        result.state = "Parsed With Findings";
        result.execution.disposition = SfzImportExecutionDisposition::completed;
    }
    else
    {
        result.state = "Parsed";
        result.execution.disposition = SfzImportExecutionDisposition::completed;
    }

    context.reportProgress(SfzImportStage::parsing, 0.30f);

    return result;
}

SfzDocumentNormalizeResult normalizeSfzDocument(const SfzParsedDocument& document)
{
    return normalizeSfzDocument(document, defaultSfzImportExecutionContext());
}

SfzDocumentNormalizeResult normalizeSfzDocument(const SfzParsedDocument& document,
                                                const SfzImportExecutionContext& context)
{
    SfzDocumentNormalizeResult result;
    context.reportProgress(SfzImportStage::normalizing, 0.30f);
    const auto initialCancellationReason = context.pollCancellation();
    if (initialCancellationReason != SfzImportCancellationReason::none)
    {
        result.execution.disposition = SfzImportExecutionDisposition::canceled;
        result.execution.cancellationReason = initialCancellationReason;
        result.state = "Canceled";
        context.reportProgress(SfzImportStage::canceled, 0.50f);
        return result;
    }
    result.document.rootDocumentPath = document.rootDocumentPath;
    result.document.sourceFiles = document.sourceFiles;
    result.state = "Normalizing";

    ResolvedOpcodeMap activeControl;
    ResolvedOpcodeMap activeGlobal;
    ResolvedOpcodeMap activeMaster;
    ResolvedOpcodeMap activeGroup;

    result.document.sections.reserve(document.sections.size());

    for (const auto& parsedSection : document.sections)
    {
        const auto cancellationReason = context.pollCancellation();
        if (cancellationReason != SfzImportCancellationReason::none)
        {
            result.execution.disposition = SfzImportExecutionDisposition::canceled;
            result.execution.cancellationReason = cancellationReason;
            result.state = "Canceled";
            result.document.sections.clear();
            context.reportProgress(SfzImportStage::canceled, 0.50f);
            return result;
        }

        auto canceled = false;
        const auto localMap = buildLocalOpcodeMap(parsedSection, context, canceled);
        if (canceled)
        {
            const auto reason = context.pollCancellation();
            result.execution.disposition = SfzImportExecutionDisposition::canceled;
            result.execution.cancellationReason = reason == SfzImportCancellationReason::none
                ? SfzImportCancellationReason::requested
                : reason;
            result.state = "Canceled";
            result.document.sections.clear();
            context.reportProgress(SfzImportStage::canceled, 0.50f);
            return result;
        }
        ResolvedOpcodeMap effective;

        switch (parsedSection.scope)
        {
            case SfzOpcodeScope::control:
                overlayOpcodes(activeControl, localMap, false, context, canceled);
                effective = activeControl;
                break;

            case SfzOpcodeScope::global:
                overlayOpcodes(activeGlobal, localMap, false, context, canceled);
                overlayOpcodes(effective, activeControl, true, context, canceled);
                overlayOpcodes(effective, activeGlobal, false, context, canceled);
                break;

            case SfzOpcodeScope::master:
                activeMaster = localMap;
                overlayOpcodes(effective, activeControl, true, context, canceled);
                overlayOpcodes(effective, activeGlobal, true, context, canceled);
                overlayOpcodes(effective, activeMaster, false, context, canceled);
                break;

            case SfzOpcodeScope::group:
                activeGroup = localMap;
                overlayOpcodes(effective, activeControl, true, context, canceled);
                overlayOpcodes(effective, activeGlobal, true, context, canceled);
                overlayOpcodes(effective, activeMaster, true, context, canceled);
                overlayOpcodes(effective, activeGroup, false, context, canceled);
                break;

            case SfzOpcodeScope::region:
                overlayOpcodes(effective, activeControl, true, context, canceled);
                overlayOpcodes(effective, activeGlobal, true, context, canceled);
                overlayOpcodes(effective, activeMaster, true, context, canceled);
                overlayOpcodes(effective, activeGroup, true, context, canceled);
                overlayOpcodes(effective, localMap, false, context, canceled);
                break;

            case SfzOpcodeScope::curve:
            case SfzOpcodeScope::effect:
            case SfzOpcodeScope::midi:
            case SfzOpcodeScope::sample:
            case SfzOpcodeScope::unknown:
                effective = localMap;
                break;
        }

        if (canceled)
        {
            const auto reason = context.pollCancellation();
            result.execution.disposition = SfzImportExecutionDisposition::canceled;
            result.execution.cancellationReason = reason == SfzImportCancellationReason::none
                ? SfzImportCancellationReason::requested
                : reason;
            result.state = "Canceled";
            result.document.sections.clear();
            context.reportProgress(SfzImportStage::canceled, 0.50f);
            return result;
        }

        SfzNormalizedSection normalizedSection;
        normalizedSection.scope = parsedSection.scope;
        normalizedSection.headerName = parsedSection.headerName;
        normalizedSection.headerLocation = parsedSection.headerLocation;
        normalizedSection.documentOrder = parsedSection.documentOrder;
        normalizedSection.localOpcodes = toResolvedOpcodeVector(localMap);
        normalizedSection.effectiveOpcodes = toResolvedOpcodeVector(effective);
        normalizedSection.localOpcodeCount = normalizedSection.localOpcodes.size();
        normalizedSection.inheritedOpcodeCount = normalizedSection.effectiveOpcodes.size() >= normalizedSection.localOpcodeCount
            ? normalizedSection.effectiveOpcodes.size() - normalizedSection.localOpcodeCount
            : 0;
        result.document.sections.push_back(std::move(normalizedSection));
    }

    result.normalized = true;
    result.execution.disposition = SfzImportExecutionDisposition::completed;
    result.state = "Normalized";
    context.reportProgress(SfzImportStage::normalizing, 0.50f);
    return result;
}

const SfzResolvedOpcode* findEffectiveOpcode(const SfzNormalizedSection& section,
                                             const std::string& opcodeName) noexcept
{
    const auto lowered = toLowerAscii(opcodeName);
    const auto iterator = std::find_if(section.effectiveOpcodes.begin(),
                                       section.effectiveOpcodes.end(),
                                       [&](const SfzResolvedOpcode& opcode)
                                       {
                                           return opcode.name == lowered;
                                       });
    return iterator == section.effectiveOpcodes.end() ? nullptr : &(*iterator);
}
} // namespace drs::engine
