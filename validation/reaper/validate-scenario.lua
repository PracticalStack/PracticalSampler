local script_path = debug.getinfo(1, "S").source:sub(2)
local output_dir = script_path:match("^(.*)[/\\][^/\\]+$")
local _, project_path = reaper.EnumProjects(-1, "")
local project_name = project_path:match("([^/\\]+)%.rpp$") or "unknown"
local log_path = output_dir .. "\\" .. project_name .. ".reaper-evidence.txt"
local chunk_path = output_dir .. "\\" .. project_name .. ".restored-track-chunks.txt"
local started = reaper.time_precise()
local duplicate_prepared = false

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
    "track_count=" .. tostring(reaper.CountTracks(0))
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
