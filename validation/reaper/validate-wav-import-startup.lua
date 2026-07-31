local script_path = debug.getinfo(1, "S").source:sub(2)
local output_dir = script_path:match("^(.*)[/\\][^/\\]+$")
local _, project_path = reaper.EnumProjects(-1, "")
local project_name = project_path:match("([^/\\]+)%.rpp$") or "unknown"
local log_path = output_dir .. "\\" .. project_name .. ".wav-import-evidence.txt"
local chunk_path = output_dir .. "\\" .. project_name .. ".wav-import-track-chunks.txt"
local started = reaper.time_precise()
local first_ready = nil

reaper.GetSet_LoopTimeRange(true, false, 0.0, 8.0, false)
reaper.GetSetRepeat(1)
reaper.OnPlayButton()

local function write_text(path, text)
  local handle = assert(io.open(path, "wb"))
  handle:write(text)
  handle:close()
end

local function current_fx_state()
  local track_count = reaper.CountTracks(0)
  if track_count < 1 then
    return nil
  end

  local track = reaper.GetTrack(0, 0)
  if track == nil then
    return nil
  end

  local fx_count = reaper.TrackFX_GetCount(track)
  if fx_count < 1 then
    return nil
  end

  local _, fx_name = reaper.TrackFX_GetFXName(track, 0, "")
  local parameter_count = reaper.TrackFX_GetNumParams(track, 0)
  return {
    track = track,
    fx_name = fx_name,
    parameter_count = parameter_count,
    offline = reaper.TrackFX_GetOffline(track, 0),
    enabled = reaper.TrackFX_GetEnabled(track, 0),
    chain_visible = reaper.TrackFX_GetChainVisible(track)
  }
end

local function capture(fx_state)
  local lines = {
    "signed_by=Codex automated REAPER WAV startup validation",
    "captured_utc=" .. os.date("!%Y-%m-%dT%H:%M:%SZ"),
    "project_name=" .. project_name,
    "project_path=" .. project_path,
    "host_version=" .. reaper.GetAppVersion(),
    "sample_rate=" .. tostring(reaper.GetSetProjectInfo(0, "PROJECT_SRATE", 0, false)),
    "play_state=" .. tostring(reaper.GetPlayState()),
    "track_count=" .. tostring(reaper.CountTracks(0)),
    "instantiation_elapsed_ms=" .. tostring(math.floor((first_ready - started) * 1000.0 + 0.5)),
    "capture_elapsed_ms=" .. tostring(math.floor((reaper.time_precise() - started) * 1000.0 + 0.5)),
    "fx_name=" .. fx_state.fx_name,
    "parameter_count=" .. tostring(fx_state.parameter_count),
    "enabled=" .. tostring(fx_state.enabled),
    "offline=" .. tostring(fx_state.offline),
    "chain_visible=" .. tostring(fx_state.chain_visible)
  }

  for parameter_index = 0, math.min(fx_state.parameter_count, 8) - 1 do
    local value = reaper.TrackFX_GetParamNormalized(fx_state.track, 0, parameter_index)
    local _, parameter_name = reaper.TrackFX_GetParamName(fx_state.track, 0, parameter_index, "")
    table.insert(lines, "parameter." .. parameter_index .. "=" .. parameter_name .. "|" .. tostring(value))
  end

  local ok, chunk = reaper.GetTrackStateChunk(fx_state.track, "", false)
  table.insert(lines, "track_chunk_captured=" .. tostring(ok))

  write_text(log_path, table.concat(lines, "\n") .. "\n")
  write_text(chunk_path, ok and chunk or "")
  reaper.OnStopButton()
end

local function wait_for_ready()
  local fx_state = current_fx_state()
  if fx_state ~= nil and fx_state.parameter_count > 0 and fx_state.offline == false and fx_state.enabled == true then
    if first_ready == nil then
      first_ready = reaper.time_precise()
    end

    if reaper.time_precise() - first_ready >= 1.0 then
      capture(fx_state)
      return
    end
  end

  if reaper.time_precise() - started >= 10.0 then
    error("Timed out waiting for REAPER WAV startup validation to become ready.")
  end

  reaper.defer(wait_for_ready)
end

wait_for_ready()
