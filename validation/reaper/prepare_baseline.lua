local script_path = debug.getinfo(1, "S").source:sub(2)
local output_dir = script_path:match("^(.*)[/\\][^/\\]+$")
local project_path = output_dir .. "\\baseline.rpp"
local chunk_path = output_dir .. "\\baseline-track-chunk.txt"
local log_path = output_dir .. "\\prepare-baseline.log"

local function write_text(path, text)
  local handle = assert(io.open(path, "wb"))
  handle:write(text)
  handle:close()
end

reaper.Undo_BeginBlock()
while reaper.CountTracks(0) > 0 do
  reaper.DeleteTrack(reaper.GetTrack(0, 0))
end

reaper.InsertTrackAtIndex(0, true)
local track = reaper.GetTrack(0, 0)
reaper.GetSetMediaTrackInfo_String(track, "P_NAME", "DRS baseline", true)
local fx_index = reaper.TrackFX_AddByName(
  track,
  "VST3: Decent Rhapsody Studio",
  false,
  -1)

local fx_name = ""
if fx_index >= 0 then
  local _, value = reaper.TrackFX_GetFXName(track, fx_index, "")
  fx_name = value
end

local chunk_ok, chunk = reaper.GetTrackStateChunk(track, "", false)
if chunk_ok then
  write_text(chunk_path, chunk)
end

reaper.Undo_EndBlock("Create Decent Rhapsody Studio baseline", -1)
reaper.Main_SaveProjectEx(0, project_path, 8)
write_text(
  log_path,
  "fx_index=" .. tostring(fx_index) .. "\n"
    .. "fx_name=" .. fx_name .. "\n"
    .. "chunk_ok=" .. tostring(chunk_ok) .. "\n"
    .. "project_path=" .. project_path .. "\n")

