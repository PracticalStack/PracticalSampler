local output_dir = assert(os.getenv("DRS_PHASE5_EVIDENCE_DIR"), "DRS_PHASE5_EVIDENCE_DIR is required")
local duplicate_path = assert(os.getenv("DRS_PHASE5_DUPLICATE_PATH"), "DRS_PHASE5_DUPLICATE_PATH is required")
local evidence_path = output_dir .. "\\phase5-restore-session.txt"

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

local function restore_session()
  local ok, message = pcall(function()
    assert(reaper.CountTracks(0) == 1, "the reopened project did not contain exactly one source track")
    local track = assert(reaper.GetTrack(0, 0), "the reopened source track is unavailable")
    assert(reaper.TrackFX_GetCount(track) == 1, "the reopened source track did not contain exactly one plug-in")
    local fx_index = 0
    local _, fx_name = reaper.TrackFX_GetFXName(track, fx_index, "")
    local parameter_count = reaper.TrackFX_GetNumParams(track, fx_index)
    local _, tone_name = reaper.TrackFX_GetParamName(track, fx_index, 0, "")
    local _, motion_name = reaper.TrackFX_GetParamName(track, fx_index, 1, "")
    local motion_value = reaper.TrackFX_GetParamNormalized(track, fx_index, 1)
    local editor_initially_open = reaper.TrackFX_GetFloatingWindow(track, fx_index) ~= nil

    local envelope = assert(reaper.GetFXEnvelope(track, fx_index, 0, false), "the Tone automation envelope did not reopen")
    local envelope_count = reaper.CountEnvelopePoints(envelope)
    local point0_ok, point0_time, point0_value = reaper.GetEnvelopePoint(envelope, 0)
    local point1_ok, point1_time, point1_value = reaper.GetEnvelopePoint(envelope, 1)

    reaper.TrackFX_Show(track, fx_index, 3)
    local editor_open_check = reaper.TrackFX_GetFloatingWindow(track, fx_index) ~= nil
    reaper.TrackFX_Show(track, fx_index, 2)

    reaper.SetOnlyTrackSelected(track)
    reaper.Main_OnCommand(40062, 0)
    assert(reaper.CountTracks(0) == 2, "REAPER did not create a duplicate instance")
    local duplicate = assert(reaper.GetTrack(0, 1), "the duplicate track is unavailable")
    assert(reaper.TrackFX_GetCount(duplicate) == 1, "the duplicate track did not retain the plug-in")
    local duplicate_motion_before = reaper.TrackFX_GetParamNormalized(duplicate, 0, 1)
    reaper.TrackFX_SetParamNormalized(duplicate, 0, 1, 0.11)
    local source_motion_after = reaper.TrackFX_GetParamNormalized(track, fx_index, 1)
    local duplicate_motion_after = reaper.TrackFX_GetParamNormalized(duplicate, 0, 1)
    local duplicate_independent = math.abs(source_motion_after - 0.77) < 0.0001
      and math.abs(duplicate_motion_after - 0.11) < 0.0001

    local source_chunk_ok, source_chunk = reaper.GetTrackStateChunk(track, "", false)
    local duplicate_chunk_ok, duplicate_chunk = reaper.GetTrackStateChunk(duplicate, "", false)
    assert(source_chunk_ok and duplicate_chunk_ok, "could not capture both reopened instance chunks")
    write_text(output_dir .. "\\phase5-restored-track-chunks.txt",
      "===== SOURCE =====\n" .. source_chunk .. "\n===== DUPLICATE =====\n" .. duplicate_chunk)

    reaper.Main_SaveProjectEx(0, duplicate_path, 8)
    local lines = {
      "status=pass",
      "phase=restore",
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
      "editor_initially_open=" .. bool_text(editor_initially_open),
      "editor_open_check=" .. bool_text(editor_open_check),
      "duplicate_track_count=" .. tostring(reaper.CountTracks(0)),
      "duplicate_motion_before=" .. number_text(duplicate_motion_before),
      "source_motion_after_duplicate_change=" .. number_text(source_motion_after),
      "duplicate_motion_after_change=" .. number_text(duplicate_motion_after),
      "duplicate_independent=" .. bool_text(duplicate_independent),
      "chunks_captured=" .. bool_text(source_chunk_ok and duplicate_chunk_ok),
      "duplicate_project_path=" .. duplicate_path
    }
    write_text(evidence_path, table.concat(lines, "\n") .. "\n")
  end)

  if not ok then
    fail(message)
    return
  end
  quit_after_capture()
end

local started = reaper.time_precise()
local function wait_for_restore()
  if reaper.time_precise() - started < 5.0 then
    reaper.defer(wait_for_restore)
    return
  end
  restore_session()
end

wait_for_restore()
