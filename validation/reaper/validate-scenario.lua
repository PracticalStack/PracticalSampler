local script_path = debug.getinfo(1, "S").source:sub(2)
local script_output_dir = script_path:match("^(.*)[/\\][^/\\]+$")
local output_dir = os.getenv("DRS_REAPER_EVIDENCE_DIR") or script_output_dir
local _, project_path = reaper.EnumProjects(-1, "")
local project_name = project_path:match("([^/\\]+)%.rpp$") or "unknown"
local log_path = output_dir .. "\\" .. project_name .. ".reaper-evidence.txt"
local chunk_path = output_dir .. "\\" .. project_name .. ".restored-track-chunks.txt"
local started = reaper.time_precise()
local duplicate_prepared = false
local observed_peak_left = 0.0
local observed_peak_right = 0.0
local observed_master_peak_left = 0.0
local observed_master_peak_right = 0.0
local peak_square_sum = 0.0
local peak_probe_count = 0
local nonzero_peak_observations = 0
local nonfinite_peak_observations = 0

local function observe_peak(value)
  if value ~= value or value == math.huge or value == -math.huge then
    nonfinite_peak_observations = nonfinite_peak_observations + 1
    return 0.0
  end
  local magnitude = math.abs(value)
  peak_square_sum = peak_square_sum + magnitude * magnitude
  peak_probe_count = peak_probe_count + 1
  if magnitude > 0.000001 then
    nonzero_peak_observations = nonzero_peak_observations + 1
  end
  return magnitude
end

local function insert_validation_midi()
  local track = reaper.GetTrack(0, 0)
  if track == nil then
    return false
  end
  local item = reaper.CreateNewMIDIItemInProj(track, 0.25, 4.0, false)
  if item == nil then
    return false
  end
  local take = reaper.GetActiveTake(item)
  if take == nil then
    return false
  end
  reaper.MIDI_InsertNote(take, false, false, 0, 3840, 0, 60, 100, false)
  reaper.MIDI_Sort(take)
  return true
end

local midi_inserted = insert_validation_midi()

reaper.GetSet_LoopTimeRange(true, false, 0.0, 60.0, false)
reaper.GetSetRepeat(1)
reaper.OnPlayButton()

local function write_text(path, text)
  local handle = assert(io.open(path, "wb"))
  handle:write(text)
  handle:close()
end

local function prepare_duplicate_instance()
  if not project_name:find("duplicate%-instances") or duplicate_prepared then
    return false
  end
  duplicate_prepared = true
  if reaper.CountTracks(0) > 1 then
    return false
  end
  local source = reaper.GetTrack(0, 0)
  reaper.SetOnlyTrackSelected(source)
  reaper.Main_OnCommand(40062, 0)
  local destination = reaper.GetTrack(0, 1)
  if destination ~= nil then
    reaper.GetSetMediaTrackInfo_String(destination, "P_NAME", "DRS duplicate", true)
  end
  reaper.Main_SaveProject(0, false)
  return true
end

local function capture()
  local lines = {
    "signed_by=Codex automated REAPER validation",
    "captured_utc=" .. os.date("!%Y-%m-%dT%H:%M:%SZ"),
    "project_name=" .. project_name,
    "project_path=" .. project_path,
    "sample_rate=" .. tostring(reaper.GetSetProjectInfo(0, "PROJECT_SRATE", 0, false)),
    "play_state=" .. tostring(reaper.GetPlayState()),
    "track_count=" .. tostring(reaper.CountTracks(0)),
    "validation_midi_inserted=" .. tostring(midi_inserted),
    "track_peak_left=" .. tostring(observed_peak_left),
    "track_peak_right=" .. tostring(observed_peak_right),
    "master_peak_left=" .. tostring(observed_master_peak_left),
    "master_peak_right=" .. tostring(observed_master_peak_right),
    "peak_rms_proxy=" .. tostring(peak_probe_count > 0
      and math.sqrt(peak_square_sum / peak_probe_count) or 0.0),
    "peak_probe_count=" .. tostring(peak_probe_count),
    "nonzero_peak_observations=" .. tostring(nonzero_peak_observations),
    "nonfinite_peak_observations=" .. tostring(nonfinite_peak_observations)
  }
  local chunks = {}

  for track_index = 0, reaper.CountTracks(0) - 1 do
    local track = reaper.GetTrack(0, track_index)
    local fx_count = reaper.TrackFX_GetCount(track)
    table.insert(lines, "track." .. track_index .. ".fx_count=" .. tostring(fx_count))
    for fx_index = 0, fx_count - 1 do
      local _, fx_name = reaper.TrackFX_GetFXName(track, fx_index, "")
      local parameter_count = reaper.TrackFX_GetNumParams(track, fx_index)
      table.insert(lines, "track." .. track_index .. ".fx." .. fx_index .. ".name=" .. fx_name)
      table.insert(lines, "track." .. track_index .. ".fx." .. fx_index .. ".parameter_count=" .. tostring(parameter_count))
      table.insert(lines, "track." .. track_index .. ".fx." .. fx_index .. ".enabled="
        .. tostring(reaper.TrackFX_GetEnabled(track, fx_index)))
      table.insert(lines, "track." .. track_index .. ".fx." .. fx_index .. ".offline="
        .. tostring(reaper.TrackFX_GetOffline(track, fx_index)))
      for parameter_index = 0, math.min(parameter_count, 8) - 1 do
        local value = reaper.TrackFX_GetParamNormalized(track, fx_index, parameter_index)
        local _, parameter_name = reaper.TrackFX_GetParamName(track, fx_index, parameter_index, "")
        table.insert(lines, "track." .. track_index .. ".fx." .. fx_index .. ".parameter."
          .. parameter_index .. "=" .. parameter_name .. "|" .. tostring(value))
      end

      if project_name:find("editor%-open")
          or project_name:find("moved%-project")
          or project_name:find("changed%-manifest")
          or project_name:find("missing%-sample") then
        reaper.TrackFX_Show(track, fx_index, 3)
      else
        reaper.TrackFX_Show(track, fx_index, 2)
      end
      table.insert(lines, "track." .. track_index .. ".fx." .. fx_index .. ".chain_visible="
        .. tostring(reaper.TrackFX_GetChainVisible(track)))
      table.insert(lines, "track." .. track_index .. ".fx." .. fx_index .. ".floating_window="
        .. tostring(reaper.TrackFX_GetFloatingWindow(track, fx_index) ~= nil))
    end

    local ok, chunk = reaper.GetTrackStateChunk(track, "", false)
    table.insert(lines, "track." .. track_index .. ".chunk_captured=" .. tostring(ok))
    if ok then
      table.insert(chunks, "===== TRACK " .. track_index .. " =====\n" .. chunk)
    end
  end

  write_text(log_path, table.concat(lines, "\n") .. "\n")
  write_text(chunk_path, table.concat(chunks, "\n"))
  reaper.OnStopButton()
end

local function wait_for_restore()
  local track = reaper.GetTrack(0, 0)
  if track ~= nil then
    observed_peak_left = math.max(observed_peak_left,
      observe_peak(reaper.Track_GetPeakInfo(track, 0)))
    observed_peak_right = math.max(observed_peak_right,
      observe_peak(reaper.Track_GetPeakInfo(track, 1)))
    local master = reaper.GetMasterTrack(0)
    observed_master_peak_left = math.max(observed_master_peak_left,
      observe_peak(reaper.Track_GetPeakInfo(master, 0)))
    observed_master_peak_right = math.max(observed_master_peak_right,
      observe_peak(reaper.Track_GetPeakInfo(master, 1)))
  end
  if reaper.time_precise() - started >= 15.0 then
    if prepare_duplicate_instance() then
      started = reaper.time_precise()
      reaper.defer(wait_for_restore)
      return
    end
    capture()
    return
  end
  reaper.defer(wait_for_restore)
end

wait_for_restore()
