obs = obslua

-- Global variables
control_window = nil
lyrics_source = nil
current_source_name = ""

function script_description()
    return [[Lyrics Floating Controls

This script creates a floating control window for the Lyrics Overlay.
The window stays on top and provides quick access to playback controls.

Note: Since OBS Lua doesn't support dock widgets, this creates a 
separate window that you can position next to your Properties panel.]]
end

-- Create HTML for the control window
function create_control_html()
    return [[
<!DOCTYPE html>
<html>
<head>
<style>
body {
    font-family: Arial, sans-serif;
    background-color: #2b2b2b;
    color: #ffffff;
    margin: 0;
    padding: 10px;
    user-select: none;
}
.container {
    width: 300px;
}
.header {
    font-size: 14px;
    font-weight: bold;
    margin-bottom: 10px;
    text-align: center;
    padding: 5px;
    background-color: #3a3a3a;
    border-radius: 3px;
}
.control-group {
    margin: 10px 0;
    padding: 10px;
    background-color: #3a3a3a;
    border-radius: 3px;
}
.control-label {
    font-size: 12px;
    margin-bottom: 5px;
    color: #cccccc;
}
select, button {
    width: 100%;
    padding: 5px;
    margin: 3px 0;
    background-color: #4a4a4a;
    color: #ffffff;
    border: 1px solid #5a5a5a;
    border-radius: 3px;
    cursor: pointer;
}
select:hover, button:hover {
    background-color: #5a5a5a;
}
button:active {
    background-color: #6a6a6a;
}
.button-row {
    display: flex;
    gap: 5px;
    margin-top: 10px;
}
.button-row button {
    flex: 1;
}
.status {
    font-size: 11px;
    color: #aaaaaa;
    text-align: center;
    margin-top: 10px;
    padding: 5px;
    background-color: #1a1a1a;
    border-radius: 3px;
}
.info {
    font-size: 10px;
    color: #888888;
    text-align: center;
    margin-top: 5px;
}
</style>
<script>
function sendCommand(cmd) {
    // Commands will be handled by the Lua script
    window.location.href = 'obs-lyrics-control://' + cmd;
}
</script>
</head>
<body>
<div class="container">
    <div class="header">Lyrics Control Panel</div>
    
    <div class="control-group">
        <div class="control-label">Source:</div>
        <select id="source-select" onchange="sendCommand('source:' + this.value)">
            <option value="">Select a source...</option>
        </select>
    </div>
    
    <div class="control-group">
        <div class="control-label">Song:</div>
        <select id="song-select" onchange="sendCommand('song:' + this.value)">
            <option value="">Select a song...</option>
        </select>
        
        <div class="button-row">
            <button onclick="sendCommand('prev')">◀ Previous</button>
            <button onclick="sendCommand('stop')">■ Stop</button>
            <button onclick="sendCommand('next')">▶ Next</button>
        </div>
    </div>
    
    <div class="status" id="status">No source selected</div>
    <div class="info">Position this window next to Properties</div>
</div>
</body>
</html>
]]
end

-- Unfortunately, OBS Lua doesn't have native window creation capabilities
-- So we'll need to use a different approach

-- Alternative: Create a simple control interface using OBS properties
function script_properties()
    local props = obs.obs_properties_create()
    
    obs.obs_properties_add_text(props, "header", "━━━ Lyrics Quick Controls ━━━", obs.OBS_TEXT_INFO)
    
    -- Source selection
    local sources = obs.obs_enum_sources()
    local source_list = obs.obs_properties_add_list(props, "source", "Lyrics Source", 
                                                   obs.OBS_COMBO_TYPE_LIST, 
                                                   obs.OBS_COMBO_FORMAT_STRING)
    
    obs.obs_property_list_add_string(source_list, "None", "")
    
    if sources then
        for _, source in ipairs(sources) do
            local source_id = obs.obs_source_get_id(source)
            if source_id == "lyrics_overlay_enhanced" then
                local name = obs.obs_source_get_name(source)
                obs.obs_property_list_add_string(source_list, name, name)
            end
        end
        obs.source_list_release(sources)
    end
    
    obs.obs_property_set_modified_callback(source_list, source_changed)
    
    -- Song selection
    obs.obs_properties_add_list(props, "song_select", "Current Song", 
                               obs.OBS_COMBO_TYPE_LIST, 
                               obs.OBS_COMBO_FORMAT_STRING)
    
    -- Quick info
    obs.obs_properties_add_text(props, "info", "💡 Keep this script window open next to Properties for quick access", obs.OBS_TEXT_INFO)
    
    -- Control buttons in a row
    obs.obs_properties_add_text(props, "controls_header", "━━━ Playback Controls ━━━", obs.OBS_TEXT_INFO)
    obs.obs_properties_add_button(props, "btn_prev", "◀ Previous Lyric", button_prev_clicked)
    obs.obs_properties_add_button(props, "btn_stop", "■ Stop/Clear", button_stop_clicked)
    obs.obs_properties_add_button(props, "btn_next", "▶ Next Lyric", button_next_clicked)
    
    -- Status
    obs.obs_properties_add_text(props, "status", "Status: Ready", obs.OBS_TEXT_INFO)
    
    -- Hotkey info
    obs.obs_properties_add_text(props, "hotkey_info", "Configure hotkeys in Settings → Hotkeys → Lyrics", obs.OBS_TEXT_INFO)
    
    return props
end

function script_defaults(settings)
    obs.obs_data_set_default_string(settings, "source", "")
end

function script_update(settings)
    current_source_name = obs.obs_data_get_string(settings, "source")
    
    if lyrics_source then
        obs.obs_source_release(lyrics_source)
        lyrics_source = nil
    end
    
    if current_source_name ~= "" then
        lyrics_source = get_source_by_name(current_source_name)
    end
    
    -- Update selected song if changed
    local selected_song = obs.obs_data_get_string(settings, "song_select")
    if selected_song ~= "" and lyrics_source then
        change_song(selected_song)
    end
end

function script_load(settings)
    -- Register hotkeys
    local hotkey_next_id = obs.obs_hotkey_register_frontend("lyrics_quick_next", "Lyrics: Next (Quick)", next_hotkey)
    local hotkey_prev_id = obs.obs_hotkey_register_frontend("lyrics_quick_prev", "Lyrics: Previous (Quick)", prev_hotkey)
    local hotkey_stop_id = obs.obs_hotkey_register_frontend("lyrics_quick_stop", "Lyrics: Stop (Quick)", stop_hotkey)
end

function script_unload()
    if lyrics_source then
        obs.obs_source_release(lyrics_source)
    end
end

-- Helper functions
function get_source_by_name(name)
    local sources = obs.obs_enum_sources()
    if sources then
        for _, source in ipairs(sources) do
            if obs.obs_source_get_name(source) == name then
                obs.source_list_release(sources)
                return source
            end
        end
        obs.source_list_release(sources)
    end
    return nil
end

function trigger_action(action)
    if not lyrics_source then
        return
    end
    
    -- Get all lyrics instances and trigger the action
    local sources = obs.obs_enum_sources()
    if sources then
        for _, source in ipairs(sources) do
            if obs.obs_source_get_name(source) == current_source_name then
                local source_id = obs.obs_source_get_id(source)
                if source_id == "lyrics_overlay_enhanced" then
                    -- We'll use a workaround by simulating button clicks
                    -- This is not ideal but works within Lua limitations
                    local settings = obs.obs_source_get_settings(source)
                    
                    if action == "next" then
                        obs.obs_data_set_bool(settings, "_next_trigger", true)
                    elseif action == "prev" then
                        obs.obs_data_set_bool(settings, "_prev_trigger", true)
                    elseif action == "stop" then
                        obs.obs_data_set_bool(settings, "_stop_trigger", true)
                    end
                    
                    obs.obs_source_update(source, settings)
                    obs.obs_data_release(settings)
                end
            end
        end
        obs.source_list_release(sources)
    end
end

function change_song(song_path)
    if lyrics_source then
        local settings = obs.obs_source_get_settings(lyrics_source)
        obs.obs_data_set_string(settings, "song_selection", song_path)
        obs.obs_source_update(lyrics_source, settings)
        obs.obs_data_release(settings)
    end
end

-- Callbacks
function source_changed(props, property, settings)
    local source_name = obs.obs_data_get_string(settings, "source")
    
    if lyrics_source then
        obs.obs_source_release(lyrics_source)
        lyrics_source = nil
    end
    
    if source_name ~= "" then
        lyrics_source = get_source_by_name(source_name)
        
        -- Update song list
        if lyrics_source then
            local source_settings = obs.obs_source_get_settings(lyrics_source)
            local songs_folder = obs.obs_data_get_string(source_settings, "songs_folder")
            obs.obs_data_release(source_settings)
            
            local song_list = obs.obs_properties_get(props, "song_select")
            obs.obs_property_list_clear(song_list)
            
            if songs_folder ~= "" then
                local handle = io.popen('ls "' .. songs_folder .. '" 2>/dev/null')
                if handle then
                    for file in handle:lines() do
                        if file:match("%.txt$") then
                            local full_path = songs_folder .. "/" .. file
                            obs.obs_property_list_add_string(song_list, file, full_path)
                        end
                    end
                    handle:close()
                end
            end
        end
    end
    
    return true
end

function button_next_clicked(props, prop)
    trigger_action("next")
    return false
end

function button_prev_clicked(props, prop)
    trigger_action("prev")
    return false
end

function button_stop_clicked(props, prop)
    trigger_action("stop")
    return false
end

-- Hotkey callbacks
function next_hotkey(pressed)
    if pressed then
        trigger_action("next")
    end
end

function prev_hotkey(pressed)
    if pressed then
        trigger_action("prev")
    end
end

function stop_hotkey(pressed)
    if pressed then
        trigger_action("stop")
    end
end
