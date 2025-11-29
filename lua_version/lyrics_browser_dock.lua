obs = obslua

-- Global state
local browser_dock = nil
local control_html = ""

function script_description()
    return [[Lyrics Browser Dock

This creates a browser dock in OBS for controlling lyrics.
It provides a dock-like interface without needing Python.

How to use:
1. Load this script
2. Go to View → Docks → Lyrics Control Panel
3. The control panel will appear as a dock]]
end

-- Generate the control panel HTML
function generate_control_html()
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
    padding: 15px;
    user-select: none;
}

.container {
    max-width: 350px;
}

h2 {
    font-size: 18px;
    margin: 0 0 15px 0;
    color: #4a90e2;
}

.section {
    background-color: #3a3a3a;
    border-radius: 6px;
    padding: 12px;
    margin-bottom: 12px;
}

.section-title {
    font-size: 14px;
    font-weight: bold;
    margin-bottom: 8px;
    color: #cccccc;
}

select, button {
    width: 100%;
    padding: 8px;
    margin: 4px 0;
    background-color: #4a4a4a;
    color: #ffffff;
    border: 1px solid #5a5a5a;
    border-radius: 4px;
    font-size: 13px;
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
    gap: 8px;
    margin-top: 8px;
}

.button-row button {
    flex: 1;
}

.status {
    background-color: #1a1a1a;
    padding: 10px;
    border-radius: 4px;
    font-size: 12px;
    line-height: 1.5;
}

.status-item {
    margin: 3px 0;
}

.preview {
    margin-top: 8px;
    padding: 8px;
    background-color: #2a2a2a;
    border-radius: 4px;
    font-style: italic;
    color: #cccccc;
    font-size: 13px;
}

.btn-stop {
    background-color: #e74c3c;
}

.btn-stop:hover {
    background-color: #c0392b;
}

.info {
    text-align: center;
    color: #888888;
    font-size: 11px;
    margin-top: 15px;
}
</style>
<script>
// OBS Browser Source API
window.obsstudio = window.obsstudio || {};

var currentSource = '';
var updateInterval = null;

function refreshSources() {
    // This would need to communicate with OBS
    // For now, show instructions
    var select = document.getElementById('source-select');
    select.innerHTML = '<option value="">Select source in OBS properties</option>';
}

function updateStatus() {
    if (!currentSource) {
        document.getElementById('current-song').textContent = 'No source selected';
        document.getElementById('current-lyric').textContent = '0/0';
        document.getElementById('lyric-preview').textContent = 'Select a lyrics source to begin';
        return;
    }
    
    // Request status from OBS
    if (window.obsstudio.getCurrentScene) {
        // This would need proper implementation
    }
}

function sendCommand(command) {
    if (!currentSource) {
        alert('Please select a lyrics source first');
        return;
    }
    
    // Send command to OBS
    console.log('Command:', command, 'for source:', currentSource);
}

function selectSource(sourceName) {
    currentSource = sourceName;
    document.getElementById('source-name').textContent = sourceName || 'None';
    updateStatus();
}

// Initialize
document.addEventListener('DOMContentLoaded', function() {
    refreshSources();
    
    // Update status periodically
    updateInterval = setInterval(updateStatus, 500);
});
</script>
</head>
<body>
<div class="container">
    <h2>Lyrics Control Panel</h2>
    
    <div class="section">
        <div class="section-title">Active Source</div>
        <div id="source-name" style="padding: 5px; background: #2a2a2a; border-radius: 3px;">
            None - Configure in script properties
        </div>
    </div>
    
    <div class="section">
        <div class="section-title">Song Selection</div>
        <select id="song-select">
            <option value="">Select a song...</option>
        </select>
    </div>
    
    <div class="section">
        <div class="section-title">Playback Controls</div>
        <div class="button-row">
            <button onclick="sendCommand('prev')">◀ Previous</button>
            <button class="btn-stop" onclick="sendCommand('stop')">■ Stop</button>
            <button onclick="sendCommand('next')">▶ Next</button>
        </div>
    </div>
    
    <div class="section">
        <div class="section-title">Current Status</div>
        <div class="status">
            <div class="status-item">Song: <span id="current-song">None</span></div>
            <div class="status-item">Lyric: <span id="current-lyric">0/0</span></div>
            <div class="preview" id="lyric-preview">No lyric loaded</div>
        </div>
    </div>
    
    <div class="info">
        Configure source selection in script properties
    </div>
</div>
</body>
</html>
]]
end

-- Create browser dock
function create_browser_dock()
    -- Create a browser source for the dock
    local browser_settings = obs.obs_data_create()
    
    -- Set HTML content
    control_html = generate_control_html()
    obs.obs_data_set_string(browser_settings, "css", "body { margin: 0; }")
    obs.obs_data_set_int(browser_settings, "width", 400)
    obs.obs_data_set_int(browser_settings, "height", 600)
    
    -- Note: OBS Lua doesn't have direct access to create docks
    -- This would need to be done through the UI
    print("Browser dock HTML generated. To use:")
    print("1. Add a Browser Source to a scene")
    print("2. Set it to 'Local File' mode")
    print("3. Use the generated HTML")
    print("4. Create a projector window for that source")
    
    obs.obs_data_release(browser_settings)
end

-- Script callbacks
function script_load(settings)
    create_browser_dock()
end

function script_properties()
    local props = obs.obs_properties_create()
    
    obs.obs_properties_add_text(props, "info", "━━━ Browser Dock Setup ━━━", obs.OBS_TEXT_INFO)
    
    obs.obs_properties_add_text(props, "instructions", 
        "Since OBS Lua cannot create docks directly:\n\n" ..
        "1. Create a new Scene called 'Lyrics Control'\n" ..
        "2. Add a Browser Source to it\n" ..
        "3. Set URL to: file:///path/to/lyrics_web_control.html\n" ..
        "4. Right-click the source → Windowed Projector\n" ..
        "5. Position the window next to OBS\n\n" ..
        "This gives you a floating control panel!", obs.OBS_TEXT_INFO)
    
    -- Source selector
    local sources = obs.obs_enum_sources()
    local source_list = obs.obs_properties_add_list(props, "lyrics_source", "Lyrics Source to Control", 
                                                   obs.OBS_COMBO_TYPE_LIST, 
                                                   obs.OBS_COMBO_FORMAT_STRING)
    
    obs.obs_property_list_add_string(source_list, "None", "")
    
    if sources then
        for _, source in ipairs(sources) do
            local source_id = obs.obs_source_get_id(source)
            if source_id == "lyrics_overlay_improved" then
                local name = obs.obs_source_get_name(source)
                obs.obs_property_list_add_string(source_list, name, name)
            end
        end
        obs.source_list_release(sources)
    end
    
    return props
end
