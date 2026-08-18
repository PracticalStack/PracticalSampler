#include "drs/engine/AuthoringPreviewCommandAdapter.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drs::engine
{
namespace
{
bool validNoteCommand(const AuthoringPreviewCommand& command) noexcept
{
    const auto validAuditionRegion = !command.hasAuditionRegion
        || (command.auditionStartFrame < command.auditionEndFrameExclusive
            && (!command.auditionLoopEnabled
                || (command.auditionLoopStartFrame >= command.auditionStartFrame
                    && command.auditionLoopStartFrame < command.auditionLoopEndFrameExclusive
                    && command.auditionLoopEndFrameExclusive <= command.auditionEndFrameExclusive)));
    return command.midiNote >= 0 && command.midiNote <= 127
        && std::isfinite(command.velocity)
        && command.velocity > 0.0f && command.velocity <= 1.0f
        && validAuditionRegion;
}
} // namespace

std::size_t AuthoringPreviewCommandAdapter::sourceIndex(
    AuthoringPreviewAuditionSource source) noexcept
{
    return std::min<std::size_t>(static_cast<std::size_t>(source), sourceCount - 1);
}

AuthoringPreviewCommandDispatch AuthoringPreviewCommandAdapter::dispatch(
    const AuthoringPreviewCommand& command) noexcept
{
    switch (command.type)
    {
        case AuthoringPreviewCommandType::noteOn:
            return dispatchNoteOn(command);
        case AuthoringPreviewCommandType::noteOff:
            return dispatchNoteOff(command);
        case AuthoringPreviewCommandType::auditionSelectedZone:
        case AuthoringPreviewCommandType::auditionSelectedGroup:
        case AuthoringPreviewCommandType::auditionCurrentDraft:
        {
            if (command.type == AuthoringPreviewCommandType::auditionSelectedZone
                && command.selectedZoneId.empty())
            {
                AuthoringPreviewCommandDispatch rejected;
                countRejected(rejected, "preview-command-selection-missing");
                return rejected;
            }
            if (command.type == AuthoringPreviewCommandType::auditionSelectedGroup
                && command.selectedGroupId.empty())
            {
                AuthoringPreviewCommandDispatch rejected;
                countRejected(rejected, "preview-command-group-missing");
                return rejected;
            }

            AuthoringPreviewCommandDispatch result;
            result.accepted = true;
            result.preparationRequested = true;
            result.requestedScope = command.type == AuthoringPreviewCommandType::auditionCurrentDraft
                ? AuthoringPreviewScope::currentDraft
                : (command.type == AuthoringPreviewCommandType::auditionSelectedGroup
                       ? AuthoringPreviewScope::selectedGroup
                       : AuthoringPreviewScope::selectedZone);
            if (command.emitNote)
            {
                auto note = dispatchNoteOn(command);
                if (!note.accepted)
                    return note;
                result.hasEvent = note.hasEvent;
                result.event = note.event;
                // dispatchNoteOn owns the accepted/event counters for combined commands.
                return result;
            }
            countAccepted(result);
            return result;
        }
        case AuthoringPreviewCommandType::stopAll:
        {
            clearOwnership();
            AuthoringPreviewCommandDispatch result;
            result.accepted = true;
            result.hasEvent = true;
            result.event = { AuthoringPreviewEventType::allNotesOff, 0, 0.0f,
                             command.sampleOffset };
            ++counters.stopCount;
            countAccepted(result);
            return result;
        }
        case AuthoringPreviewCommandType::emergencyReset:
        {
            clearOwnership();
            AuthoringPreviewCommandDispatch result;
            result.accepted = true;
            result.hasEvent = true;
            result.event = { AuthoringPreviewEventType::reset, 0, 0.0f,
                             command.sampleOffset };
            ++counters.resetCount;
            countAccepted(result);
            return result;
        }
    }

    AuthoringPreviewCommandDispatch rejected;
    countRejected(rejected, "preview-command-type-invalid");
    return rejected;
}

AuthoringPreviewCommandDispatch AuthoringPreviewCommandAdapter::dispatchNoteOn(
    const AuthoringPreviewCommand& command) noexcept
{
    AuthoringPreviewCommandDispatch result;
    if (!validNoteCommand(command))
    {
        countRejected(result, "preview-command-note-invalid");
        return result;
    }

    const auto source = sourceIndex(command.source);
    const auto note = static_cast<std::size_t>(command.midiNote);
    if (ownershipBySource[source][note] == std::numeric_limits<std::uint16_t>::max()
        || totalOwnershipByNote[note] == std::numeric_limits<std::uint16_t>::max())
    {
        countRejected(result, "preview-command-ownership-saturated");
        return result;
    }

    ++ownershipBySource[source][note];
    ++totalOwnershipByNote[note];
    result.accepted = true;
    result.hasEvent = true;
    result.event.type = AuthoringPreviewEventType::noteOn;
    result.event.midiNote = command.midiNote;
    result.event.velocity = command.velocity;
    result.event.sampleOffset = command.sampleOffset;
    result.event.hasAuditionRegion = command.hasAuditionRegion;
    result.event.auditionStartFrame = command.auditionStartFrame;
    result.event.auditionEndFrameExclusive = command.auditionEndFrameExclusive;
    result.event.auditionLoopEnabled = command.auditionLoopEnabled;
    result.event.auditionLoopStartFrame = command.auditionLoopStartFrame;
    result.event.auditionLoopEndFrameExclusive = command.auditionLoopEndFrameExclusive;
    countAccepted(result);
    return result;
}

AuthoringPreviewCommandDispatch AuthoringPreviewCommandAdapter::dispatchNoteOff(
    const AuthoringPreviewCommand& command) noexcept
{
    AuthoringPreviewCommandDispatch result;
    if (command.midiNote < 0 || command.midiNote > 127)
    {
        countRejected(result, "preview-command-note-invalid");
        return result;
    }

    const auto source = sourceIndex(command.source);
    const auto note = static_cast<std::size_t>(command.midiNote);
    if (ownershipBySource[source][note] == 0 || totalOwnershipByNote[note] == 0)
    {
        countRejected(result, "preview-command-note-not-owned");
        return result;
    }

    --ownershipBySource[source][note];
    --totalOwnershipByNote[note];
    result.accepted = true;
    if (totalOwnershipByNote[note] == 0)
    {
        result.hasEvent = true;
        result.event = { AuthoringPreviewEventType::noteOff, command.midiNote, 0.0f,
                         command.sampleOffset };
    }
    else
    {
        ++counters.suppressedNoteOffCount;
    }
    countAccepted(result);
    return result;
}

void AuthoringPreviewCommandAdapter::countAccepted(
    AuthoringPreviewCommandDispatch& dispatch) noexcept
{
    dispatch.accepted = true;
    ++counters.acceptedCommandCount;
    if (dispatch.hasEvent)
        ++counters.emittedEventCount;
}

void AuthoringPreviewCommandAdapter::countRejected(
    AuthoringPreviewCommandDispatch& dispatch,
    const char* code) noexcept
{
    dispatch.accepted = false;
    dispatch.rejectionCode = code;
    ++counters.rejectedCommandCount;
}

AuthoringPreviewCommandAdapterSnapshot AuthoringPreviewCommandAdapter::getSnapshot() const noexcept
{
    auto snapshot = counters;
    snapshot.ownedNoteCount = 0;
    snapshot.distinctOwnedNoteCount = 0;
    for (const auto count : totalOwnershipByNote)
    {
        snapshot.ownedNoteCount += count;
        if (count != 0)
            ++snapshot.distinctOwnedNoteCount;
    }
    return snapshot;
}

void AuthoringPreviewCommandAdapter::clearOwnership() noexcept
{
    for (auto& source : ownershipBySource)
        source.fill(0);
    totalOwnershipByNote.fill(0);
}
} // namespace drs::engine
