local output_dir = assert(os.getenv("DRS_PHASE5_EVIDENCE_DIR"), "DRS_PHASE5_EVIDENCE_DIR is required")
local project_path = assert(os.getenv("DRS_PHASE5_PROJECT_PATH"), "DRS_PHASE5_PROJECT_PATH is required")
local evidence_path = output_dir .. "\\phase5-create-session.txt"

local function write_text(path, text)
  local handle = assert(io.open(path, "wb"))
  handle:write(text)
  handle:close()
end

local function bool_text(value)
  return value and "true" or "false"
end

local function number_text(value)
  return string.format("%.12f", value)
end

local function quit_after_capture()
  reaper.Main_OnCommand(40004, 0)
end

local function fail(message)
  write_text(evidence_path, "status=fail\nerror=" .. tostring(message) .. "\n")
  quit_after_capture()
end

local session = {}

local function capture_session()
  local ok, message = pcall(function()
    local track = session.track
    local fx_index = session.fx_index
    local envelope = session.envelope
    local editor_open = reaper.TrackFX_GetFloatingWindow(track, fx_index) ~= nil
    local _, fx_name = reaper.TrackFX_GetFXName(track, fx_index, "")
    local parameter_count = reaper.TrackFX_GetNumParams(track, fx_index)
    local _, tone_name = reaper.TrackFX_GetParamName(track, fx_index, 0, "")
    local _, motion_name = reaper.TrackFX_GetParamName(track, fx_index, 1, "")
    local motion_value = reaper.TrackFX_GetParamNormalized(track, fx_index, 1)
    local envelope_count = reaper.CountEnvelopePoints(envelope)
    local point0_ok, point0_time, point0_value = reaper.GetEnvelopePoint(envelope, 0)
    local point1_ok, point1_time, point1_value = reaper.GetEnvelopePoint(envelope, 1)

    reaper.TrackFX_Show(track, fx_index, 2)
    local saved_editor_closed = reaper.TrackFX_GetFloatingWindow(track, fx_index) == nil
    reaper.Main_SaveProjectEx(0, project_path, 8)
    local chunk_ok, chunk = reaper.GetTrackStateChunk(track, "", false)
    assert(chunk_ok, "could not capture the saved track chunk")
    write_text(output_dir .. "\\phase5-created-track-chunk.txt", chunk)

    local lines = {
      "status=pass",
      "phase=create",
      "fx_name=" .. fx_name,
      "fx_count=" .. tostring(reaper.TrackFX_GetCount(track)),
      "parameter_count=" .. tostring(parameter_count),
      "tone_name=" .. tone_name,
      "motion_name=" .. motion_name,
      "motion_value=" .. number_text(motion_value),
      "automation_point_count=" .. tostring(envelope_count),
      "automation_point_0_ok=" .. bool_text(point0_ok),
      "automation_point_0_time=" .. number_text(point0_time),
      "automation_point_0_value=" .. number_text(point0_value),
      "automation_point_1_ok=" .. bool_text(point1_ok),
      "automation_point_1_time=" .. number_text(point1_time),
      "automation_point_1_value=" .. number_text(point1_value),
      "editor_open=" .. bool_text(editor_open),
      "saved_editor_closed=" .. bool_text(saved_editor_closed),
      "project_path=" .. project_path,
      "chunk_captured=" .. bool_text(chunk_ok)
    }
    write_text(evidence_path, table.concat(lines, "\n") .. "\n")
  end)

  if not ok then
    fail(message)
    return
  end
  quit_after_capture()
end

local function wait_for_parameter_settle()
  if reaper.time_precise() - session.parameters_set_at < 2.0 then
    reaper.defer(wait_for_parameter_settle)
    return
  end
  capture_session()
end

local function configure_session()
  local ok, message = pcall(function()
    local track = session.track
    local fx_index = session.fx_index
    reaper.TrackFX_SetParamNormalized(track, fx_index, 0, 0.23)
    reaper.TrackFX_SetParamNormalized(track, fx_index, 1, 0.77)

    local envelope = assert(reaper.GetFXEnvelope(track, fx_index, 0, true), "could not create Tone automation")
    reaper.DeleteEnvelopePointRange(envelope, -1000000.0, 1000000.0)
    assert(reaper.InsertEnvelopePoint(envelope, 0.0, 0.23, 0, 0.0, false, true), "could not insert the first automation point")
    assert(reaper.InsertEnvelopePoint(envelope, 2.0, 0.81, 0, 0.0, false, true), "could not insert the second automation point")
    reaper.Envelope_SortPoints(envelope)
    reaper.TrackFX_Show(track, fx_index, 3)
    session.envelope = envelope
    session.parameters_set_at = reaper.time_precise()
  end)
  if not ok then
    fail(message)
    return
  end
  reaper.defer(wait_for_parameter_settle)
end

local function wait_for_plugin_initialization()
  if reaper.time_precise() - session.plugin_added_at < 4.0 then
    reaper.defer(wait_for_plugin_initialization)
    return
  end
  configure_session()
end

local function create_session()
  local ok, message = pcall(function()
    while reaper.CountTracks(0) > 0 do
      reaper.DeleteTrack(reaper.GetTrack(0, 0))
    end

    reaper.InsertTrackAtIndex(0, true)
    local track = assert(reaper.GetTrack(0, 0), "could not create the qualification track")
    reaper.GetSetMediaTrackInfo_String(track, "P_NAME", "Practical Sampler Phase 5", true)
    local fx_index = reaper.TrackFX_AddByName(track, "VST3: Practical Sampler", false, -1)
    assert(fx_index >= 0, "Practical Sampler was not discovered in the isolated scan path")

    local item = assert(reaper.CreateNewMIDIItemInProj(track, 0.25, 2.75, false), "could not create qualification MIDI")
    local take = assert(reaper.GetActiveTake(item), "could not obtain the qualification MIDI take")
    local note_start = reaper.MIDI_GetPPQPosFromProjTime(take, 0.5)
    local note_end = reaper.MIDI_GetPPQPosFromProjTime(take, 1.5)
    assert(reaper.MIDI_InsertNote(take, false, false, note_start, note_end, 0, 60, 100, false), "could not insert qualification MIDI")
    reaper.MIDI_Sort(take)

    session.track = track
    session.fx_index = fx_index
    session.plugin_added_at = reaper.time_precise()
  end)
  if not ok then
    fail(message)
    return
  end
  reaper.defer(wait_for_plugin_initialization)
end

local started = reaper.time_precise()
local function wait_for_scan()
  if reaper.time_precise() - started < 3.0 then
    reaper.defer(wait_for_scan)
    return
  end
  create_session()
end

wait_for_scan()
