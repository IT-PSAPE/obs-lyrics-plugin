obs = obslua

-- Global variables
lyrics_instances = {}
instance_counter = 0

-- LyricsSource class
local LyricsSource = {}
LyricsSource.__index = LyricsSource

function LyricsSource:new(source, settings)
    local self = setmetatable({}, LyricsSource)
    self.source = source
    self.settings = settings
    self.instance_id = instance_counter
    instance_counter = instance_counter + 1
    
    -- File and folder settings
    self.image_path = ""
    self.songs_folder = ""
    self.current_song_file = ""
    self.lyrics_lines = {}
    self.current_line_index = 0
    self.is_visible = true
    
    -- Sources
    self.text_source = nil
    self.image_source = nil
    
    -- Dimensions
    self.width = 1920
    self.height = 1080
    
    -- Song management
    self.available_songs = {}
    self.current_song_index = -1
    
    -- Register instance
    lyrics_instances[self.instance_id] = self
    
    -- Initialize text source with proper settings
    local text_settings = obs.obs_data_create()
    obs.obs_data_set_string(text_settings, "text", "Click Next Lyric to begin")
    obs.obs_data_set_int(text_settings, "color1", 0xFFFFFFFF)  -- White text
    obs.obs_data_set_int(text_settings, "color2", 0xFFFFFFFF)
    
    -- Set default font
    local font_obj = obs.obs_data_create()
    obs.obs_data_set_string(font_obj, "face", "Arial")
    obs.obs_data_set_int(font_obj, "size", 48)
    obs.obs_data_set_string(font_obj, "style", "Regular")
    obs.obs_data_set_obj(text_settings, "font", font_obj)
    obs.obs_data_release(font_obj)
    
    -- Set alignment
    obs.obs_data_set_string(text_settings, "align", "center")
    obs.obs_data_set_string(text_settings, "valign", "center")
    
    self.text_source = obs.obs_source_create_private("text_ft2_source", "lyrics_text_" .. self.instance_id, text_settings)
    obs.obs_data_release(text_settings)
    
    -- Initialize image source
    local img_settings = obs.obs_data_create()
    self.image_source = obs.obs_source_create_private("image_source", "lyrics_bg_" .. self.instance_id, img_settings)
    obs.obs_data_release(img_settings)
    
    return self
end

function LyricsSource:update(settings)
    self.settings = settings
    self.image_path = obs.obs_data_get_string(settings, "background_image")
    self.songs_folder = obs.obs_data_get_string(settings, "songs_folder")
    local new_song_file = obs.obs_data_get_string(settings, "song_selection")
    
    -- Check for control triggers from external scripts
    if obs.obs_data_get_bool(settings, "_next_trigger") then
        obs.obs_data_set_bool(settings, "_next_trigger", false)
        self:next_lyric()
    end
    
    if obs.obs_data_get_bool(settings, "_prev_trigger") then
        obs.obs_data_set_bool(settings, "_prev_trigger", false)
        self:prev_lyric()
    end
    
    if obs.obs_data_get_bool(settings, "_stop_trigger") then
        obs.obs_data_set_bool(settings, "_stop_trigger", false)
        self:stop_lyrics()
    end
    
    -- Check if song changed
    local song_changed = (new_song_file ~= self.current_song_file)
    self.current_song_file = new_song_file
    
    -- Update background image
    if self.image_path ~= "" and self.image_source then
        local img_settings = obs.obs_data_create()
        obs.obs_data_set_string(img_settings, "file", self.image_path)
        obs.obs_source_update(self.image_source, img_settings)
        obs.obs_data_release(img_settings)
    end
    
    -- Load available songs
    self:load_available_songs()
    
    -- Update text appearance
    self:update_text_appearance()
    
    -- Load lyrics if song changed
    if song_changed and self.current_song_file ~= "" then
        self:load_lyrics(self.current_song_file)
    end
end

function LyricsSource:update_text_appearance()
    if not self.text_source then
        return
    end
    
    local text_settings = obs.obs_data_create()
    
    -- Font settings
    local font_size = obs.obs_data_get_int(self.settings, "font_size")
    if font_size == 0 then font_size = 48 end
    
    local font_face = obs.obs_data_get_string(self.settings, "font_face")
    if font_face == "" then font_face = "Arial" end
    
    local font_style = obs.obs_data_get_string(self.settings, "font_style")
    if font_style == "" then font_style = "Regular" end
    
    local font_obj = obs.obs_data_create()
    obs.obs_data_set_string(font_obj, "face", font_face)
    obs.obs_data_set_int(font_obj, "size", font_size)
    obs.obs_data_set_string(font_obj, "style", font_style)
    obs.obs_data_set_obj(text_settings, "font", font_obj)
    
    -- Set current text
    local current_text = self:get_current_lyric()
    if current_text == "" and self.current_line_index == 0 and #self.lyrics_lines == 0 then
        current_text = "No lyrics loaded. Select a song and click Next Lyric."
    end
    obs.obs_data_set_string(text_settings, "text", current_text)
    
    -- Text alignment
    local h_align = obs.obs_data_get_string(self.settings, "h_align")
    local v_align = obs.obs_data_get_string(self.settings, "v_align")
    if h_align == "" then h_align = "center" end
    if v_align == "" then v_align = "center" end
    obs.obs_data_set_string(text_settings, "align", h_align)
    obs.obs_data_set_string(text_settings, "valign", v_align)
    
    -- Text color - ensure it's visible
    local text_color = obs.obs_data_get_int(self.settings, "text_color")
    if text_color == 0 then text_color = 0xFFFFFFFF end  -- Default to white
    obs.obs_data_set_int(text_settings, "color1", text_color)
    obs.obs_data_set_int(text_settings, "color2", text_color)
    
    -- Outline settings
    local outline_enabled = obs.obs_data_get_bool(self.settings, "outline_enabled")
    obs.obs_data_set_bool(text_settings, "outline", outline_enabled)
    if outline_enabled then
        local outline_size = obs.obs_data_get_int(self.settings, "outline_size")
        if outline_size == 0 then outline_size = 2 end
        local outline_color = obs.obs_data_get_int(self.settings, "outline_color")
        if outline_color == 0 then outline_color = 0xFF000000 end
        obs.obs_data_set_int(text_settings, "outline_size", outline_size)
        obs.obs_data_set_int(text_settings, "outline_color", outline_color)
    end
    
    -- Shadow settings
    local shadow_enabled = obs.obs_data_get_bool(self.settings, "shadow_enabled")
    obs.obs_data_set_bool(text_settings, "drop_shadow", shadow_enabled)
    if shadow_enabled then
        local shadow_color = obs.obs_data_get_int(self.settings, "shadow_color")
        if shadow_color == 0 then shadow_color = 0xFF000000 end
        obs.obs_data_set_int(text_settings, "shadow_color", shadow_color)
    end
    
    -- Word wrap
    obs.obs_data_set_bool(text_settings, "word_wrap", true)
    local text_width = obs.obs_data_get_int(self.settings, "text_width")
    if text_width == 0 then text_width = 1600 end
    obs.obs_data_set_int(text_settings, "custom_width", text_width)
    
    -- Apply settings
    obs.obs_source_update(self.text_source, text_settings)
    obs.obs_data_release(text_settings)
    obs.obs_data_release(font_obj)
end

function LyricsSource:load_available_songs()
    self.available_songs = {}
    if self.songs_folder ~= "" then
        local handle = io.popen('ls "' .. self.songs_folder .. '" 2>/dev/null')
        if handle then
            for file in handle:lines() do
                if file:match("%.txt$") then
                    table.insert(self.available_songs, file)
                end
            end
            handle:close()
            table.sort(self.available_songs)
            
            if self.current_song_file ~= "" then
                local song_name = self.current_song_file:match("([^/]+)$")
                for i, v in ipairs(self.available_songs) do
                    if v == song_name then
                        self.current_song_index = i
                        break
                    end
                end
            end
        end
    end
end

function LyricsSource:load_lyrics(file_path)
    self.lyrics_lines = {}
    local file = io.open(file_path, "r")
    if file then
        for line in file:lines() do
            table.insert(self.lyrics_lines, line)
        end
        file:close()
    end
    self.current_line_index = 0
    self:update_text_display()
end

function LyricsSource:get_current_lyric()
    if not self.is_visible then
        return ""
    end
    if self.current_line_index >= 1 and self.current_line_index <= #self.lyrics_lines then
        return self.lyrics_lines[self.current_line_index]
    end
    return ""
end

function LyricsSource:video_render(effect)
    if not self.is_visible then
        return
    end
    
    -- IMPORTANT: Draw background first, then text on top
    if self.image_source then
        obs.obs_source_video_render(self.image_source)
    end
    
    -- Draw text on top of background
    if self.text_source then
        local offset_x = obs.obs_data_get_int(self.settings, "text_offset_x")
        local offset_y = obs.obs_data_get_int(self.settings, "text_offset_y")
        
        obs.gs_matrix_push()
        obs.gs_matrix_translate3f(offset_x, offset_y, 0.0)
        obs.obs_source_video_render(self.text_source)
        obs.gs_matrix_pop()
    end
end

function LyricsSource:video_tick(seconds)
    -- Force update text visibility
    if self.text_source then
        local width = obs.obs_source_get_width(self.text_source)
        local height = obs.obs_source_get_height(self.text_source)
        if width == 0 or height == 0 then
            self:update_text_display()
        end
    end
end

function LyricsSource:next_lyric()
    if self.current_line_index < #self.lyrics_lines then
        self.current_line_index = self.current_line_index + 1
        self:update_text_display()
        return true
    end
    return false
end

function LyricsSource:prev_lyric()
    if self.current_line_index > 0 then
        self.current_line_index = self.current_line_index - 1
        self:update_text_display()
        return true
    end
    return false
end

function LyricsSource:stop_lyrics()
    self.current_line_index = 0
    self:update_text_display()
end

function LyricsSource:update_text_display()
    if self.text_source then
        local settings = obs.obs_data_create()
        local text = self:get_current_lyric()
        if text == "" and self.current_line_index == 0 and #self.lyrics_lines > 0 then
            text = "[Ready - Click Next to start]"
        elseif text == "" and #self.lyrics_lines == 0 then
            text = "[No song loaded]"
        end
        obs.obs_data_set_string(settings, "text", text)
        obs.obs_source_update(self.text_source, settings)
        obs.obs_data_release(settings)
    end
end

function LyricsSource:get_width()
    if self.image_source then
        local w = obs.obs_source_get_width(self.image_source)
        if w > 0 then return w end
    end
    return self.width
end

function LyricsSource:get_height()
    if self.image_source then
        local h = obs.obs_source_get_height(self.image_source)
        if h > 0 then return h end
    end
    return self.height
end

function LyricsSource:get_info()
    local song_name = "No song"
    if self.current_song_file ~= "" then
        song_name = self.current_song_file:match("([^/]+)$") or "Unknown"
    end
    
    return {
        song_name = song_name,
        current_line = self.current_line_index,
        total_lines = #self.lyrics_lines,
        current_lyric = self:get_current_lyric()
    }
end

function LyricsSource:destroy()
    lyrics_instances[self.instance_id] = nil
    if self.text_source then
        obs.obs_source_release(self.text_source)
    end
    if self.image_source then
        obs.obs_source_release(self.image_source)
    end
end

-- -------------------------------------------------------------------
-- Script Functions
-- -------------------------------------------------------------------

function script_description()
    return [[Lyrics Overlay Plugin

Features:
• Load background images
• Load songs from a folder (txt files)
• Each line in txt file is a separate slide
• Customizable text appearance
• Text always renders on top of background

Usage:
1. Add "Lyrics Overlay" source to your scene
2. Configure in source properties
3. Use playback controls to navigate]]
end

function script_load(settings)
    obs.obs_register_source(source_info)
end

function script_unload()
    for _, instance in pairs(lyrics_instances) do
        instance:destroy()
    end
    lyrics_instances = {}
end

-- -------------------------------------------------------------------
-- Property Callbacks
-- -------------------------------------------------------------------

function on_songs_folder_modified(props, property, settings)
    local songs_folder = obs.obs_data_get_string(settings, "songs_folder")
    
    local song_list = obs.obs_properties_get(props, "song_selection")
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
    
    return true
end

-- Button callbacks
function on_next_lyric_clicked(props, prop)
    for _, instance in pairs(lyrics_instances) do
        instance:next_lyric()
    end
    return true
end

function on_prev_lyric_clicked(props, prop)
    for _, instance in pairs(lyrics_instances) do
        instance:prev_lyric()
    end
    return true
end

function on_stop_clicked(props, prop)
    for _, instance in pairs(lyrics_instances) do
        instance:stop_lyrics()
    end
    return true
end

-- -------------------------------------------------------------------
-- Source Definition
-- -------------------------------------------------------------------

source_info = {}
source_info.id = "lyrics_overlay_enhanced"
source_info.type = obs.OBS_SOURCE_TYPE_INPUT
source_info.output_flags = obs.OBS_SOURCE_VIDEO + obs.OBS_SOURCE_CUSTOM_DRAW

source_info.get_name = function()
    return "Lyrics Overlay"
end

source_info.create = function(settings, source)
    return LyricsSource:new(source, settings)
end

source_info.destroy = function(data)
    data:destroy()
end

source_info.update = function(data, settings)
    data:update(settings)
end

source_info.video_render = function(data, effect)
    data:video_render(effect)
end

source_info.video_tick = function(data, seconds)
    data:video_tick(seconds)
end

source_info.get_width = function(data)
    return data:get_width()
end

source_info.get_height = function(data)
    return data:get_height()
end

source_info.get_properties = function(data)
    local props = obs.obs_properties_create()
    
    -- File Management
    obs.obs_properties_add_text(props, "file_header", "━━━ File Management ━━━", obs.OBS_TEXT_INFO)
    
    obs.obs_properties_add_path(props, "background_image", "Background Image", 
                               obs.OBS_PATH_FILE, 
                               "Images (*.png *.jpg *.jpeg *.gif *.bmp)", "")
    
    local songs_folder_prop = obs.obs_properties_add_path(props, "songs_folder", "Songs Folder", 
                                                         obs.OBS_PATH_DIRECTORY, "", "")
    obs.obs_property_set_modified_callback(songs_folder_prop, on_songs_folder_modified)
    
    local song_list = obs.obs_properties_add_list(props, "song_selection", "Current Song", 
                                                 obs.OBS_COMBO_TYPE_LIST, 
                                                 obs.OBS_COMBO_FORMAT_STRING)
    
    if data and data.songs_folder ~= "" then
        local handle = io.popen('ls "' .. data.songs_folder .. '" 2>/dev/null')
        if handle then
            for file in handle:lines() do
                if file:match("%.txt$") then
                    local full_path = data.songs_folder .. "/" .. file
                    obs.obs_property_list_add_string(song_list, file, full_path)
                end
            end
            handle:close()
        end
    end
    
    -- Text Appearance
    obs.obs_properties_add_text(props, "appearance_header", "━━━ Text Appearance ━━━", obs.OBS_TEXT_INFO)
    
    -- Font settings
    local font_list = obs.obs_properties_add_list(props, "font_face", "Font", 
                                                 obs.OBS_COMBO_TYPE_EDITABLE, 
                                                 obs.OBS_COMBO_FORMAT_STRING)
    obs.obs_property_list_add_string(font_list, "Arial", "Arial")
    obs.obs_property_list_add_string(font_list, "Helvetica", "Helvetica")
    obs.obs_property_list_add_string(font_list, "Times New Roman", "Times New Roman")
    obs.obs_property_list_add_string(font_list, "Verdana", "Verdana")
    
    obs.obs_properties_add_int(props, "font_size", "Font Size", 12, 200, 2)
    
    local style_list = obs.obs_properties_add_list(props, "font_style", "Font Style", 
                                                  obs.OBS_COMBO_TYPE_LIST, 
                                                  obs.OBS_COMBO_FORMAT_STRING)
    obs.obs_property_list_add_string(style_list, "Regular", "Regular")
    obs.obs_property_list_add_string(style_list, "Bold", "Bold")
    obs.obs_property_list_add_string(style_list, "Italic", "Italic")
    obs.obs_property_list_add_string(style_list, "Bold Italic", "Bold Italic")
    
    obs.obs_properties_add_color(props, "text_color", "Text Color")
    
    -- Text alignment
    local h_align_list = obs.obs_properties_add_list(props, "h_align", "Horizontal Align", 
                                                    obs.OBS_COMBO_TYPE_LIST, 
                                                    obs.OBS_COMBO_FORMAT_STRING)
    obs.obs_property_list_add_string(h_align_list, "Left", "left")
    obs.obs_property_list_add_string(h_align_list, "Center", "center")
    obs.obs_property_list_add_string(h_align_list, "Right", "right")
    
    local v_align_list = obs.obs_properties_add_list(props, "v_align", "Vertical Align", 
                                                    obs.OBS_COMBO_TYPE_LIST, 
                                                    obs.OBS_COMBO_FORMAT_STRING)
    obs.obs_property_list_add_string(v_align_list, "Top", "top")
    obs.obs_property_list_add_string(v_align_list, "Center", "center")
    obs.obs_property_list_add_string(v_align_list, "Bottom", "bottom")
    
    -- Text Effects
    obs.obs_properties_add_text(props, "effects_header", "━━━ Text Effects ━━━", obs.OBS_TEXT_INFO)
    
    -- Outline settings
    obs.obs_properties_add_bool(props, "outline_enabled", "Enable Outline")
    obs.obs_properties_add_int(props, "outline_size", "Outline Size", 1, 20, 1)
    obs.obs_properties_add_color(props, "outline_color", "Outline Color")
    
    -- Shadow settings
    obs.obs_properties_add_bool(props, "shadow_enabled", "Enable Shadow")
    obs.obs_properties_add_int(props, "shadow_opacity", "Shadow Opacity (%)", 0, 100, 5)
    obs.obs_properties_add_int(props, "shadow_offset_x", "Shadow X Offset", -100, 100, 1)
    obs.obs_properties_add_int(props, "shadow_offset_y", "Shadow Y Offset", -100, 100, 1)
    obs.obs_properties_add_color(props, "shadow_color", "Shadow Color")
    
    -- Text Position
    obs.obs_properties_add_text(props, "position_header", "━━━ Text Position ━━━", obs.OBS_TEXT_INFO)
    
    obs.obs_properties_add_int(props, "text_offset_x", "X Offset", -5000, 5000, 10)
    obs.obs_properties_add_int(props, "text_offset_y", "Y Offset", -5000, 5000, 10)
    obs.obs_properties_add_int(props, "text_width", "Text Width", 100, 5000, 50)
    
    -- Playback Controls
    obs.obs_properties_add_text(props, "controls_header", "━━━ Playback Controls ━━━", obs.OBS_TEXT_INFO)
    
    -- Status display
    if data then
        local info = data:get_info()
        local status = string.format("Song: %s | Lyric: %d/%d", 
                                   info.song_name, info.current_line, info.total_lines)
        obs.obs_properties_add_text(props, "status", status, obs.OBS_TEXT_INFO)
        
        if info.current_lyric ~= "" then
            local preview = info.current_lyric
            if #preview > 50 then
                preview = string.sub(preview, 1, 50) .. "..."
            end
            obs.obs_properties_add_text(props, "preview", "Current: " .. preview, obs.OBS_TEXT_INFO)
        end
    end
    
    -- Control buttons
    obs.obs_properties_add_button(props, "btn_prev_lyric", "◀ Previous Lyric", on_prev_lyric_clicked)
    obs.obs_properties_add_button(props, "btn_stop", "■ Stop/Clear", on_stop_clicked)
    obs.obs_properties_add_button(props, "btn_next_lyric", "▶ Next Lyric", on_next_lyric_clicked)
    
    return props
end
