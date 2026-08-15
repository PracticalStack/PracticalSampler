#include "drs/engine/ContinuousDamperContract.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;

struct ControllerEvent
{
    int track = 0;
    std::int64_t tick = 0;
    int channel = 0;
    int value = 0;
};

constexpr std::array<std::int64_t, 28> expectedTicks {
    0, 6130, 6167, 6207, 6242, 6280, 6357, 6398, 6432, 6477, 6515, 6552, 6630, 6668,
    6707, 7163, 7202, 7240, 7280, 7317, 7352, 7430, 7467, 7508, 7560, 7600, 7637, 7677
};

constexpr std::array<int, 28> expectedValues {
    0, 12, 46, 56, 60, 61, 60, 58, 57, 54, 52, 52, 50, 15,
    0, 22, 48, 55, 58, 59, 59, 60, 62, 63, 64, 66, 68, 72
};

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string readText(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Could not read " + path.generic_string());
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

void requireContains(const std::string& text,
                     const std::string& marker,
                     const std::string& message)
{
    require(text.find(marker) != std::string::npos, message + " (missing '" + marker + "').");
}

int readMidiFormat(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    std::array<unsigned char, 10> header {};
    input.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
    require(input.gcount() == static_cast<std::streamsize>(header.size()),
            "The source MIDI must contain a complete header");
    require(std::equal(header.begin(), header.begin() + 4,
                       std::array<unsigned char, 4> { 'M', 'T', 'h', 'd' }.begin()),
            "The source MIDI must start with an MThd chunk");
    return (static_cast<int>(header[8]) << 8) | static_cast<int>(header[9]);
}

std::vector<ControllerEvent> readCc64Trace(const fs::path& path,
                                           int& format,
                                           int& trackCount,
                                           int& timeFormat,
                                           std::string& sha256)
{
    const juce::File file(path.string());
    require(file.existsAsFile(), "The checked-in Accurate Salamander MIDI must exist");

    format = readMidiFormat(path);

    juce::FileInputStream hashInput(file);
    require(hashInput.openedOk(), "The Accurate Salamander MIDI must open for hashing");
    sha256 = juce::SHA256(hashInput).toHexString().toStdString();

    juce::FileInputStream midiInput(file);
    require(midiInput.openedOk(), "The Accurate Salamander MIDI must open for parsing");
    juce::MidiFile midi;
    require(midi.readFrom(midiInput), "The Accurate Salamander MIDI must parse");

    trackCount = midi.getNumTracks();
    timeFormat = midi.getTimeFormat();

    std::vector<ControllerEvent> events;
    for (int trackIndex = 0; trackIndex < midi.getNumTracks(); ++trackIndex)
    {
        const auto* track = midi.getTrack(trackIndex);
        require(track != nullptr, "Every declared MIDI track must be readable");
        for (int eventIndex = 0; eventIndex < track->getNumEvents(); ++eventIndex)
        {
            const auto* holder = track->getEventPointer(eventIndex);
            require(holder != nullptr, "Every MIDI event holder must be readable");
            const auto& message = holder->message;
            if (message.isController() && message.getControllerNumber() == 64)
            {
                events.push_back({ trackIndex + 1,
                                   static_cast<std::int64_t>(std::llround(message.getTimeStamp())),
                                   message.getChannel(),
                                   message.getControllerValue() });
            }
        }
    }
    return events;
}

void verifyContractConstants()
{
    using namespace drs::engine;
    require(continuousDamperProjectSchemaVersion == 7,
            "HP-01 must reserve project schema 7");
    require(continuousDamperAuthoringSchemaVersion == 6,
            "HP-01 must reserve authoring schema 6");
    require(continuousDamperInstrumentSchemaVersion == 5,
            "HP-01 must reserve instrument schema 5");
    require(legacySustainControllerNumber == 64 && legacySustainThreshold == 64.0,
            "Legacy native content must retain binary CC64 at threshold 64");
    require(sfzDefaultSustainControllerNumber == 64 && sfzDefaultSustainThreshold == 0.5,
            "Imported SFZ must retain the ARIA sustain defaults");
    require(halfPedalReleaseControllerNumber == 64,
            "The focused half-pedal release controller must be CC64");
    require(continuousDamperCurvePointCount == 128,
            "Compiled damper curves must contain exactly 128 values");
    require(minimumDynamicReleaseSeconds == 0.001
                && maximumDynamicReleaseSeconds == 100.0,
            "Dynamic release duration bounds must remain frozen");
}

void verifySyntheticFixtures(const fs::path& fixtureRoot)
{
    const auto synthetic = readText(fixtureRoot / "synthetic-looped-piano.fixture.json");
    requireContains(synthetic, "\"fixtureId\": \"continuous-damper-synthetic-looped-piano\"",
                    "Synthetic piano fixture identity must remain stable");
    requireContains(synthetic, "\"sustainControllerNumber\": 90",
                    "Synthetic piano must separate binary sustain from CC64 release control");
    requireContains(synthetic, "\"controllerNumber\": 64",
                    "Synthetic piano must use CC64 release control");
    requireContains(synthetic, "\"amountSeconds\": 100.0",
                    "Synthetic piano must retain the Salamander release amount");
    requireContains(synthetic, "\"v032\": 0.0",
                    "Synthetic piano must retain Salamander curve point v032");
    requireContains(synthetic, "\"v064\": 1.0",
                    "Synthetic piano must retain Salamander curve point v064");

    const auto timelines = readText(fixtureRoot / "continuous-damper-timelines.json");
    requireContains(timelines, "\"fixtureId\": \"continuous-damper-hp01-timelines\"",
                    "Timeline fixture identity must remain stable");
    for (const auto* id : { "continuous-tail-ladder", "repedal-catch",
                            "release-trigger-uniqueness", "sustain-controller-reassignment",
                            "activation-generation-cutover", "legacy-binary-compatibility" })
        requireContains(timelines, std::string("\"id\": \"") + id + "\"",
                        "Every HP-01 timeline must remain checked in");
    requireContains(timelines, "catch-remaining-level:110",
                    "Repedal must catch the remaining level");
    requireContains(timelines, "do-not-restore-lost-energy",
                    "Repedal must not restore lost energy");
    requireContains(timelines, "route-release-sample:60:once",
                    "Repedaling must not duplicate release samples");
    requireContains(timelines, "update-note:60-with-origin-generation:41",
                    "Old-generation voices must retain their release-control owner");
    requireContains(timelines, "fixed-authored-release-unchanged",
                    "Legacy binary playback must remain unchanged");
}

void verifyRealMidiTrace(const fs::path& fixtureRoot, const fs::path& workspaceRoot)
{
    const auto trace = readText(fixtureRoot / "accurate-salamander-tests-cc64-trace.json");
    requireContains(trace, "\"cc64EventCount\": 28",
                    "The trace must declare all 28 CC64 events");
    requireContains(trace, "\"distinctValueCount\": 22",
                    "The trace must declare 22 distinct CC64 values");
    requireContains(trace, "\"belowLegacyThresholdCount\": 24",
                    "The trace must expose the legacy binary cutoff");
    requireContains(trace, "values 12 through 61 and back to 0 never enter current DRS sustain",
                    "The trace must record why the first passage currently cuts off");

    juce::var parsedTrace;
    const auto parseResult = juce::JSON::parse(juce::String::fromUTF8(trace.data(),
                                                                      static_cast<int>(trace.size())),
                                                parsedTrace);
    require(parseResult.wasOk(), "The checked-in CC64 trace must be valid JSON");
    const auto* traceObject = parsedTrace.getDynamicObject();
    require(traceObject != nullptr, "The checked-in CC64 trace must be an object");
    const auto* fixtureEvents = traceObject->getProperty("events").getArray();
    require(fixtureEvents != nullptr && fixtureEvents->size() == static_cast<int>(expectedValues.size()),
            "The checked-in trace must retain exactly 28 event objects");
    for (int index = 0; index < fixtureEvents->size(); ++index)
    {
        const auto* eventObject = fixtureEvents->getReference(index).getDynamicObject();
        require(eventObject != nullptr, "Every checked-in trace event must be an object");
        require(static_cast<int>(eventObject->getProperty("tick")) == expectedTicks[static_cast<std::size_t>(index)]
                    && static_cast<int>(eventObject->getProperty("value")) == expectedValues[static_cast<std::size_t>(index)],
                "The checked-in CC64 trace sequence changed at index " + std::to_string(index));
    }

    int format = 0;
    int trackCount = 0;
    int timeFormat = 0;
    std::string sha256;
    const auto events = readCc64Trace(workspaceRoot / "DemoMidi/AccurateSalamanderTests.mid",
                                      format, trackCount, timeFormat, sha256);
    require(format == 1, "The source MIDI must remain format 1");
    require(trackCount == 2, "The source MIDI must retain two tracks");
    require(timeFormat == 960, "The source MIDI must remain 960 PPQ");
    require(sha256 == "f47ee4961578c087909100a8ab1b8160f22e4df9ab993ed3c91159152de13a62",
            "The source MIDI hash must match the frozen HP-01 trace");
    require(events.size() == expectedValues.size(),
            "The source MIDI must retain exactly 28 CC64 events");

    std::set<int> distinctValues;
    for (std::size_t index = 0; index < events.size(); ++index)
    {
        require(events[index].track == 2, "Every frozen CC64 event must remain on track 2");
        require(events[index].channel == 1, "Every frozen CC64 event must remain on channel 1");
        require(events[index].tick == expectedTicks[index],
                "CC64 tick sequence changed at index " + std::to_string(index));
        require(events[index].value == expectedValues[index],
                "CC64 value sequence changed at index " + std::to_string(index));
        distinctValues.insert(events[index].value);
    }
    require(distinctValues.size() == 22, "The source MIDI must retain 22 distinct CC64 values");
    require(std::count_if(events.begin(), events.end(), [](const auto& event)
                          { return event.value < 64; }) == 24,
            "The source MIDI must retain 24 CC64 events below the legacy threshold");
}
} // namespace

int main()
{
    try
    {
        verifyContractConstants();
        const auto fixtureRoot = fs::path(DRS_CONTINUOUS_DAMPER_HP01_FIXTURE_ROOT);
        verifySyntheticFixtures(fixtureRoot);
        verifyRealMidiTrace(fixtureRoot, fs::path(DRS_WORKSPACE_ROOT));
        std::cout << "Continuous damper HP-01 contract and fixture tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Continuous damper HP-01 contract tests failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
