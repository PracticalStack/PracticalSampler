#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace
{
std::string readText(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Could not read fixture " + path.generic_string());
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireContains(const std::string& text, const std::string& token, const std::string& message)
{
    require(text.find(token) != std::string::npos, message + " (missing '" + token + "').");
}
} // namespace

int main()
{
    try
    {
        const auto root = std::filesystem::path(DRS_PERFORMANCE_ENGINE_S0_FIXTURE_ROOT);
        const auto strings = readText(root / "two-articulation-strings.fixture.json");
        const auto piano = readText(root / "piano-lite-event-timeline.json");

        requireContains(strings, "\"fixtureId\": \"two-articulation-strings\"",
                        "Strings fixture identity must remain stable");
        requireContains(strings, "\"midiNote\": 12", "Strings fixture must reserve C0/MIDI 12");
        requireContains(strings, "\"midiNote\": 14", "Strings fixture must reserve D0/MIDI 14");
        requireContains(strings, "\"select-articulation:sustain\"",
                        "C0 must select Sustain");
        requireContains(strings, "\"select-articulation:staccato\"",
                        "D0 must select Staccato");
        requireContains(strings, "\"consume-note-on:12\"",
                        "The C0 note-on must be consumed");
        requireContains(strings, "\"consume-note-off:14\"",
                        "The D0 note-off must be consumed");
        requireContains(strings, "\"strings-sustain-main\"",
                        "Strings fixture must retain the Sustain playable route");
        requireContains(strings, "\"strings-staccato-main\"",
                        "Strings fixture must retain the Staccato playable route");

        requireContains(piano, "\"fixtureId\": \"piano-lite-semantic-timeline\"",
                        "Piano timeline identity must remain stable");
        requireContains(piano, "\"route:piano-c4-key-up\"",
                        "Physical note-off must retain a key-up route");
        requireContains(piano, "\"defer-effective-release:60\"",
                        "Pedal-down note-off must defer effective release");
        requireContains(piano, "\"emit-release:60\"",
                        "Pedal-up must emit the deferred effective release");
        requireContains(piano, "\"route:piano-c4-damper\"",
                        "Effective release must route the damper sample");
        requireContains(piano, "\"route:piano-pedal-up\"",
                        "Pedal-up must route pedal noise after release routes");
        requireContains(piano, "\"sampleOffset\": 320",
                        "Repeated pedal-up must remain represented as a no-op transition");

        std::cout << "Performance-engine Sprint 0 fixture contract tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance-engine Sprint 0 fixture contract tests failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
