obs = obslua

-- Global variables
lyrics_instances = {}
instance_counter = 0

-- Default values that match what's displayed
local DEFAULTS = {
    font_size = 48,
    font_face = "Arial",
    font_style = "Regular",
    text_color = 0xFFFFFFFF,
    text_width = 800,
    text_height = 600,
    h_align = "center",
    v_align = "center",
    outline_enabled = false,
    outline_size = 2,
    outline_color = 0xFF000000,
    shadow_enabled = false,
    shadow_color = 0xFF000000,
    show_text_bounds = false,
    text_bounds_color = 0xFFFF0000
}

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
    self.bounds_source = nil  -- For showing text bounds
    
    -- Dimensions
    self.width = 1920
    self.height = 1080
    
    -- Song management
    self.available_songs = {}
    self.current_song_index = -1
    
    -- Register instance
    lyrics_instances[self.instance_id] = self
    
    -- Initialize text source with default settings
    local text_settings = obs.obs_data_create()
    self:apply_defaults(text_settings)
    obs.obs_data_set_string(text_settings, "text", "Click Next Lyric to begin")
    self.text_source = obs.obs_source_create_private("text_ft2_source", "lyrics_text_" .. self.instance_id, text_settings)
    obs.obs_data_release(text_settings)
    
    -- Initialize image source
    local img_settings = obs.obs_data_create()
    self.image_source = obs.obs_source_create_private("image_source", "lyrics_bg_" .. self.instance_id, img_settings)
    obs.obs_data_release(img_settings)
    
    -- Initialize bounds visualization source (using image source with generated color)
    -- Note: OBS Lua doesn't have direct access to color_source, so we'll draw the bounds differently
    self.show_bounds = false
    self.bounds_color = DEFAULTS.text_bounds_color
    
    return self
end

function LyricsSource:apply_defaults(settings)
    -- Apply default font
    local font_obj = obs.obs_data_create()
    obs.obs_data_set_string(font_obj, "face", DEFAULTS.font_face)
    obs.obs_data_set_int(font_obj, "size", DEFAULTS.font_size)
    obs.obs_data_set_string(font_obj, "style", DEFAULTS.font_style)
    obs.obs_data_set_obj(settings, "font", font_obj)
    obs.obs_data_release(font_obj)
    
    -- Apply default colors
    obs.obs_data_set_int(settings, "color1", DEFAULTS.text_color)
    obs.obs_data_set_int(settings, "color2", DEFAULTS.text_color)
    
    -- Apply default alignment
    obs.obs_data_set_string(settings, "align", DEFAULTS.h_align)
    obs.obs_data_set_string(settings, "valign", DEFAULTS.v_align)
    
    -- Word wrap with default width
    obs.obs_data_set_bool(settings, "word_wrap", true)
    obs.obs_data_set_int(settings, "custom_width", DEFAULTS.text_width)
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
    
    -- Update bounds visualization settings
    self.show_bounds = obs.obs_data_get_bool(settings, "show_text_bounds")
    self.bounds_color = obs.obs_data_get_int(settings, "text_bounds_color")
    
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
    
    -- Font settings with proper defaults
    local font_size = obs.obs_data_get_int(self.settings, "font_size")
    if font_size == 0 then font_size = DEFAULTS.font_size end
    
    local font_face = obs.obs_data_get_string(self.settings, "font_face")
    if font_face == "" then font_face = DEFAULTS.font_face end
    
    local font_style = obs.obs_data_get_string(self.settings, "font_style")
    if font_style == "" then font_style = DEFAULTS.font_style end
    
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
    if h_align == "" then h_align = DEFAULTS.h_align end
    if v_align == "" then v_align = DEFAULTS.v_align end
    obs.obs_data_set_string(text_settings, "align", h_align)
    obs.obs_data_set_string(text_settings, "valign", v_align)
    
    -- Text color
    local text_color = obs.obs_data_get_int(self.settings, "text_color")
    if text_color == 0 then text_color = DEFAULTS.text_color end
    obs.obs_data_set_int(text_settings, "color1", text_color)
    obs.obs_data_set_int(text_settings, "color2", text_color)
    
    -- Outline settings
    local outline_enabled = obs.obs_data_get_bool(self.settings, "outline_enabled")
    obs.obs_data_set_bool(text_settings, "outline", outline_enabled)
    if outline_enabled then
        local outline_size = obs.obs_data_get_int(self.settings, "outline_size")
        if outline_size == 0 then outline_size = DEFAULTS.outline_size end
        local outline_color = obs.obs_data_get_int(self.settings, "outline_color")
        if outline_color == 0 then outline_color = DEFAULTS.outline_color end
        obs.obs_data_set_int(text_settings, "outline_size", outline_size)
        obs.obs_data_set_int(text_settings, "outline_color", outline_color)
    end
    
    -- Shadow settings
    local shadow_enabled = obs.obs_data_get_bool(self.settings, "shadow_enabled")
    obs.obs_data_set_bool(text_settings, "drop_shadow", shadow_enabled)
    if shadow_enabled then
        local shadow_color = obs.obs_data_get_int(self.settings, "shadow_color")
        if shadow_color == 0 then shadow_color = DEFAULTS.shadow_color end
        obs.obs_data_set_int(text_settings, "shadow_color", shadow_color)
    end
    
    -- Word wrap with custom width
    obs.obs_data_set_bool(text_settings, "word_wrap", true)
    local text_width = obs.obs_data_get_int(self.settings, "text_width")
    if text_width == 0 then text_width = DEFAULTS.text_width end
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
    
    -- Draw background image first
    if self.image_source then
        obs.obs_source_video_render(self.image_source)
    end
    
    -- Get text box dimensions and position
    local text_width = obs.obs_data_get_int(self.settings, "text_width")
    local text_height = obs.obs_data_get_int(self.settings, "text_height")
    local h_align = obs.obs_data_get_string(self.settings, "h_align")
    local v_align = obs.obs_data_get_string(self.settings, "v_align")
    
    if text_width == 0 then text_width = DEFAULTS.text_width end
    if text_height == 0 then text_height = DEFAULTS.text_height end
    if h_align == "" then h_align = DEFAULTS.h_align end
    if v_align == "" then v_align = DEFAULTS.v_align end
    
    -- Calculate position based on alignment
    local x_offset = 0
    local y_offset = 0
    local source_width = self:get_width()
    local source_height = self:get_height()
    
    -- Horizontal alignment
    if h_align == "center" then
        x_offset = (source_width - text_width) / 2
    elseif h_align == "right" then
        x_offset = source_width - text_width
    end
    
    -- Vertical alignment
    if v_align == "center" then
        y_offset = (source_height - text_height) / 2
    elseif v_align == "bottom" then
        y_offset = source_height - text_height
    end
    
    -- Draw text bounds if enabled
    if self.show_bounds then
        obs.gs_matrix_push()
        obs.gs_matrix_translate3f(x_offset, y_offset, 0.0)
        
        -- Draw rectangle outline for bounds
        obs.gs_render_start(true)
        
        -- Top line
        obs.gs_vertex2f(0, 0)
        obs.gs_vertex2f(text_width, 0)
        
        -- Right line
        obs.gs_vertex2f(text_width, 0)
        obs.gs_vertex2f(text_width, text_height)
        
        -- Bottom line
        obs.gs_vertex2f(text_width, text_height)
        obs.gs_vertex2f(0, text_height)
        
        -- Left line
        obs.gs_vertex2f(0, text_height)
        obs.gs_vertex2f(0, 0)
        
        -- Set color and render
        local color = obs.vec4()
        obs.vec4_from_rgba(color, self.bounds_color)
        obs.gs_render_stop(obs.GS_LINESTRIP)
        
        -- Draw with the color
        local effect = obs.obs_get_base_effect(obs.OBS_EFFECT_SOLID)
        obs.gs_effect_set_vec4(obs.gs_effect_get_param_by_name(effect, "color"), color)
        while obs.gs_effect_loop(effect, "Solid") do
            obs.gs_draw(obs.GS_LINESTRIP, 0, 0)
        end
        
        obs.gs_matrix_pop()
    end
    
    -- Draw text on top
    if self.text_source then
        obs.gs_matrix_push()
        obs.gs_matrix_translate3f(x_offset, y_offset, 0.0)
        
        -- Create a viewport to clip text to bounds
        obs.gs_viewport_push()
        obs.gs_set_viewport(x_offset, y_offset, text_width, text_height)
        
        obs.obs_source_video_render(self.text_source)
        
        obs.gs_viewport_pop()
        obs.gs_matrix_pop()
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
    return [[Lyrics Overlay Plugin - Improved

Features:
• Background images with text overlay
• Text box with adjustable width/height
• Alignment-based positioning (no manual offsets)
• Visual text bounds indicator
• Proper default values

Text is always rendered on top of the background.]]
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
source_info.id = "lyrics_overlay_improved"
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

source_info.get_width = function(data)
    return data:get_width()
end

source_info.get_height = function(data)
    return data:get_height()
end

source_info.get_defaults = function(settings)
    -- Set all default values
    obs.obs_data_set_default_int(settings, "font_size", DEFAULTS.font_size)
    obs.obs_data_set_default_string(settings, "font_face", DEFAULTS.font_face)
    obs.obs_data_set_default_string(settings, "font_style", DEFAULTS.font_style)
    obs.obs_data_set_default_int(settings, "text_color", DEFAULTS.text_color)
    obs.obs_data_set_default_int(settings, "text_width", DEFAULTS.text_width)
    obs.obs_data_set_default_int(settings, "text_height", DEFAULTS.text_height)
    obs.obs_data_set_default_string(settings, "h_align", DEFAULTS.h_align)
    obs.obs_data_set_default_string(settings, "v_align", DEFAULTS.v_align)
    obs.obs_data_set_default_bool(settings, "outline_enabled", DEFAULTS.outline_enabled)
    obs.obs_data_set_default_int(settings, "outline_size", DEFAULTS.outline_size)
    obs.obs_data_set_default_int(settings, "outline_color", DEFAULTS.outline_color)
    obs.obs_data_set_default_bool(settings, "shadow_enabled", DEFAULTS.shadow_enabled)
    obs.obs_data_set_default_int(settings, "shadow_color", DEFAULTS.shadow_color)
    obs.obs_data_set_default_bool(settings, "show_text_bounds", DEFAULTS.show_text_bounds)
    obs.obs_data_set_default_int(settings, "text_bounds_color", DEFAULTS.text_bounds_color)
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
    
    -- Text Box Settings
    obs.obs_properties_add_text(props, "box_header", "━━━ Text Box Settings ━━━", obs.OBS_TEXT_INFO)
    
    obs.obs_properties_add_int(props, "text_width", "Text Box Width", 100, 3840, 10)
    obs.obs_properties_add_int(props, "text_height", "Text Box Height", 50, 2160, 10)
    
    -- Text Position (Alignment)
    obs.obs_properties_add_text(props, "position_header", "━━━ Text Position ━━━", obs.OBS_TEXT_INFO)
    
    local h_align_list = obs.obs_properties_add_list(props, "h_align", "Horizontal Position", 
                                                    obs.OBS_COMBO_TYPE_LIST, 
                                                    obs.OBS_COMBO_FORMAT_STRING)
    obs.obs_property_list_add_string(h_align_list, "Left", "left")
    obs.obs_property_list_add_string(h_align_list, "Center", "center")
    obs.obs_property_list_add_string(h_align_list, "Right", "right")
    
    local v_align_list = obs.obs_properties_add_list(props, "v_align", "Vertical Position", 
                                                    obs.OBS_COMBO_TYPE_LIST, 
                                                    obs.OBS_COMBO_FORMAT_STRING)
    obs.obs_property_list_add_string(v_align_list, "Top", "top")
    obs.obs_property_list_add_string(v_align_list, "Center", "center")
    obs.obs_property_list_add_string(v_align_list, "Bottom", "bottom")
    
    -- Text Box Visualization
    obs.obs_properties_add_bool(props, "show_text_bounds", "Show Text Box Outline")
    obs.obs_properties_add_color(props, "text_bounds_color", "Text Box Outline Color")
    
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
    
    -- Text Effects
    obs.obs_properties_add_text(props, "effects_header", "━━━ Text Effects ━━━", obs.OBS_TEXT_INFO)
    
    -- Outline settings
    obs.obs_properties_add_bool(props, "outline_enabled", "Enable Text Outline")
    obs.obs_properties_add_int(props, "outline_size", "Outline Size", 1, 20, 1)
    obs.obs_properties_add_color(props, "outline_color", "Outline Color")
    
    -- Shadow settings
    obs.obs_properties_add_bool(props, "shadow_enabled", "Enable Text Shadow")
    obs.obs_properties_add_color(props, "shadow_color", "Shadow Color")
    
    -- Playback Controls
    obs.obs_properties_add_text(props, "controls_header", "━━━ Playback Controls ━━━", obs.OBS_TEXT_INFO)
    
    -- Control buttons
    obs.obs_properties_add_button(props, "btn_prev_lyric", "◀ Previous Lyric", on_prev_lyric_clicked)
    obs.obs_properties_add_button(props, "btn_stop", "■ Stop/Clear", on_stop_clicked)
    obs.obs_properties_add_button(props, "btn_next_lyric", "▶ Next Lyric", on_next_lyric_clicked)
    
    return props
end
