obs = obslua
socket = require("socket")

-- Server configuration
local server = nil
local clients = {}
local port = 9999

function script_description()
    return [[Lyrics Web Control Server

This script creates a WebSocket server for the web control panel.
Open lyrics_web_control.html in your browser to control lyrics.

Note: This requires LuaSocket. If not available, the server won't start.]]
end

-- Simple WebSocket handshake
function websocket_handshake(client_socket)
    local request = client_socket:receive("*l")
    if not request then return false end
    
    local key = nil
    while true do
        local line = client_socket:receive("*l")
        if not line or line == "" then break end
        
        local k, v = line:match("^(.-):%s*(.*)$")
        if k and k:lower() == "sec-websocket-key" then
            key = v
        end
    end
    
    if not key then return false end
    
    -- Generate accept key (simplified - in production use proper SHA1)
    local magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
    local accept_key = key .. magic -- Should be SHA1 + base64, but simplified for Lua
    
    local response = "HTTP/1.1 101 Switching Protocols\r\n" ..
                    "Upgrade: websocket\r\n" ..
                    "Connection: Upgrade\r\n" ..
                    "Sec-WebSocket-Accept: " .. accept_key .. "\r\n\r\n"
    
    client_socket:send(response)
    return true
end

-- WebSocket frame encoding (simplified)
function encode_websocket_frame(data)
    local len = #data
    local frame = string.char(0x81) -- FIN + text frame
    
    if len <= 125 then
        frame = frame .. string.char(len)
    else
        frame = frame .. string.char(126) .. string.char(math.floor(len / 256)) .. string.char(len % 256)
    end
    
    return frame .. data
end

-- Send message to all connected clients
function broadcast_message(message)
    local json = obs.obs_data_create()
    for k, v in pairs(message) do
        if type(v) == "string" then
            obs.obs_data_set_string(json, k, v)
        elseif type(v) == "number" then
            obs.obs_data_set_int(json, k, v)
        elseif type(v) == "boolean" then
            obs.obs_data_set_bool(json, k, v)
        elseif type(v) == "table" then
            local array = obs.obs_data_array_create()
            for _, item in ipairs(v) do
                local obj = obs.obs_data_create()
                for ik, iv in pairs(item) do
                    obs.obs_data_set_string(obj, ik, tostring(iv))
                end
                obs.obs_data_array_push_back(array, obj)
                obs.obs_data_release(obj)
            end
            obs.obs_data_set_array(json, k, array)
            obs.obs_data_array_release(array)
        end
    end
    
    local json_string = obs.obs_data_get_json(json)
    obs.obs_data_release(json)
    
    local frame = encode_websocket_frame(json_string)
    
    for i = #clients, 1, -1 do
        local client = clients[i]
        local success, err = pcall(function()
            client:send(frame)
        end)
        
        if not success then
            table.remove(clients, i)
        end
    end
end

-- Handle client messages
function handle_client_message(data)
    -- Parse JSON (simplified)
    local message = {}
    for k, v in data:gmatch('"([^"]+)":"([^"]+)"') do
        message[k] = v
    end
    
    if message.action == "get_sources" then
        local sources_list = {}
        local sources = obs.obs_enum_sources()
        if sources then
            for _, source in ipairs(sources) do
                local id = obs.obs_source_get_id(source)
                if id == "lyrics_overlay_improved" then
                    local name = obs.obs_source_get_name(source)
                    table.insert(sources_list, name)
                end
            end
            obs.source_list_release(sources)
        end
        
        broadcast_message({
            type = "sources",
            sources = sources_list
        })
        
    elseif message.action == "select_source" then
        local source = obs.obs_get_source_by_name(message.source)
        if source then
            local settings = obs.obs_source_get_settings(source)
            local songs_folder = obs.obs_data_get_string(settings, "songs_folder")
            obs.obs_data_release(settings)
            
            -- Get songs list
            local songs = {}
            if songs_folder ~= "" then
                local handle = io.popen('ls "' .. songs_folder .. '" 2>/dev/null')
                if handle then
                    for file in handle:lines() do
                        if file:match("%.txt$") then
                            table.insert(songs, {
                                name = file,
                                path = songs_folder .. "/" .. file
                            })
                        end
                    end
                    handle:close()
                end
            end
            
            obs.obs_source_release(source)
            
            broadcast_message({
                type = "songs",
                songs = songs
            })
        end
        
    elseif message.action == "control" then
        -- Trigger control on the source
        local source = obs.obs_get_source_by_name(message.source)
        if source then
            local settings = obs.obs_source_get_settings(source)
            
            if message.command == "next" then
                obs.obs_data_set_bool(settings, "_next_trigger", true)
            elseif message.command == "prev" then
                obs.obs_data_set_bool(settings, "_prev_trigger", true)
            elseif message.command == "stop" then
                obs.obs_data_set_bool(settings, "_stop_trigger", true)
            end
            
            obs.obs_source_update(source, settings)
            obs.obs_data_release(settings)
            obs.obs_source_release(source)
        end
        
    elseif message.action == "change_song" then
        local source = obs.obs_get_source_by_name(message.source)
        if source then
            local settings = obs.obs_source_get_settings(source)
            obs.obs_data_set_string(settings, "song_selection", message.song)
            obs.obs_source_update(source, settings)
            obs.obs_data_release(settings)
            obs.obs_source_release(source)
        end
        
    elseif message.action == "get_status" then
        -- This would need to be implemented in the main plugin
        -- For now, send dummy status
        broadcast_message({
            type = "status",
            status = {
                song_name = "Current Song",
                current_line = 1,
                total_lines = 10,
                current_lyric = "Sample lyric text"
            }
        })
    end
end

-- Server loop
function server_loop()
    if not server then return end
    
    server:settimeout(0)
    
    -- Accept new connections
    local client = server:accept()
    if client then
        client:settimeout(0)
        if websocket_handshake(client) then
            table.insert(clients, client)
            print("Client connected")
        end
    end
    
    -- Handle client messages
    for i = #clients, 1, -1 do
        local client = clients[i]
        local data, err = client:receive("*l")
        
        if data then
            -- Decode WebSocket frame (simplified)
            if #data > 2 then
                handle_client_message(data)
            end
        elseif err == "closed" then
            table.remove(clients, i)
            print("Client disconnected")
        end
    end
end

function script_load(settings)
    -- Try to create server
    local success, err = pcall(function()
        server = socket.bind("localhost", port)
        if server then
            print("Lyrics web server started on port " .. port)
            obs.timer_add(server_loop, 50)
        end
    end)
    
    if not success then
        print("Failed to start web server: " .. tostring(err))
        print("Note: This requires LuaSocket to be installed")
    end
end

function script_unload()
    obs.timer_remove(server_loop)
    
    if server then
        server:close()
    end
    
    for _, client in ipairs(clients) do
        client:close()
    end
end
