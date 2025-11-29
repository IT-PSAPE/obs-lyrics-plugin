import obspython as obs
from lyrics_plugin import LyricsSource, lyrics_instances, DEFAULTS
from lyrics_dock_widget import LyricsDockWidget

# Global dock widget
dock_widget = None

# OBS Script Functions
def script_description():
    return """Lyrics Overlay Plugin - Python Version with Dock

Features:
• Dock widget for easy control (appears next to Properties/Filters)
• Background images with text overlay
• Adjustable text box with alignment-based positioning
• Visual bounds indicator for easy positioning
• Proper default values that match displayed text
• Text effects (outline, shadow)

The dock widget provides quick access to all controls without opening properties."""

def script_load(settings):
    obs.obs_register_source(source_info)
    
    # Create dock widget
    global dock_widget
    dock_widget = LyricsDockWidget()
    obs.obs_frontend_add_dock(dock_widget)
    dock_widget.refresh_sources()

def script_unload():
    global dock_widget
    if dock_widget:
        obs.obs_frontend_remove_dock(dock_widget.windowTitle())
        dock_widget = None

# Property callbacks
def on_songs_folder_modified(props, property, settings):
    songs_folder = obs.obs_data_get_string(settings, "songs_folder")
    
    song_list = obs.obs_properties_get(props, "song_selection")
    obs.obs_property_list_clear(song_list)
    
    if songs_folder and os.path.exists(songs_folder):
        import os
        files = [f for f in os.listdir(songs_folder) if f.endswith('.txt')]
        for f in sorted(files):
            full_path = os.path.join(songs_folder, f)
            obs.obs_property_list_add_string(song_list, f, full_path)
    
    return True

# Source callbacks
def source_create(settings, source):
    return LyricsSource(source, settings)

def source_destroy(data):
    data.destroy()

def source_update(data, settings):
    data.update(settings)

def source_video_render(data, effect):
    data.video_render(effect)

def source_get_width(data):
    return data.get_width()

def source_get_height(data):
    return data.get_height()

def source_get_defaults(settings):
    for key, value in DEFAULTS.items():
        if isinstance(value, bool):
            obs.obs_data_set_default_bool(settings, key, value)
        elif isinstance(value, int):
            obs.obs_data_set_default_int(settings, key, value)
        elif isinstance(value, str):
            obs.obs_data_set_default_string(settings, key, value)

def source_get_properties(data):
    props = obs.obs_properties_create()
    
    # File Management
    obs.obs_properties_add_text(props, "file_header", "━━━ File Management ━━━", obs.OBS_TEXT_INFO)
    
    obs.obs_properties_add_path(props, "background_image", "Background Image", 
                               obs.OBS_PATH_FILE, 
                               "Images (*.png *.jpg *.jpeg *.gif *.bmp)", "")
    
    songs_folder_prop = obs.obs_properties_add_path(props, "songs_folder", "Songs Folder", 
                                                   obs.OBS_PATH_DIRECTORY, "", "")
    obs.obs_property_set_modified_callback(songs_folder_prop, on_songs_folder_modified)
    
    song_list = obs.obs_properties_add_list(props, "song_selection", "Current Song", 
                                           obs.OBS_COMBO_TYPE_LIST, 
                                           obs.OBS_COMBO_FORMAT_STRING)
    
    # Text Box Settings
    obs.obs_properties_add_text(props, "box_header", "━━━ Text Box Settings ━━━", obs.OBS_TEXT_INFO)
    
    obs.obs_properties_add_int(props, "text_width", "Text Box Width", 100, 3840, 10)
    obs.obs_properties_add_int(props, "text_height", "Text Box Height", 50, 2160, 10)
    
    # Position
    obs.obs_properties_add_text(props, "position_header", "━━━ Text Position ━━━", obs.OBS_TEXT_INFO)
    
    h_align_list = obs.obs_properties_add_list(props, "h_align", "Horizontal Position", 
                                              obs.OBS_COMBO_TYPE_LIST, 
                                              obs.OBS_COMBO_FORMAT_STRING)
    obs.obs_property_list_add_string(h_align_list, "Left", "left")
    obs.obs_property_list_add_string(h_align_list, "Center", "center")
    obs.obs_property_list_add_string(h_align_list, "Right", "right")
    
    v_align_list = obs.obs_properties_add_list(props, "v_align", "Vertical Position", 
                                              obs.OBS_COMBO_TYPE_LIST, 
                                              obs.OBS_COMBO_FORMAT_STRING)
    obs.obs_property_list_add_string(v_align_list, "Top", "top")
    obs.obs_property_list_add_string(v_align_list, "Center", "center")
    obs.obs_property_list_add_string(v_align_list, "Bottom", "bottom")
    
    # Bounds visualization
    obs.obs_properties_add_bool(props, "show_text_bounds", "Show Text Box Outline")
    obs.obs_properties_add_color(props, "text_bounds_color", "Text Box Outline Color")
    
    # Text Appearance
    obs.obs_properties_add_text(props, "appearance_header", "━━━ Text Appearance ━━━", obs.OBS_TEXT_INFO)
    
    font_list = obs.obs_properties_add_list(props, "font_face", "Font", 
                                           obs.OBS_COMBO_TYPE_EDITABLE, 
                                           obs.OBS_COMBO_FORMAT_STRING)
    obs.obs_property_list_add_string(font_list, "Arial", "Arial")
    obs.obs_property_list_add_string(font_list, "Helvetica", "Helvetica")
    obs.obs_property_list_add_string(font_list, "Times New Roman", "Times New Roman")
    obs.obs_property_list_add_string(font_list, "Verdana", "Verdana")
    
    obs.obs_properties_add_int(props, "font_size", "Font Size", 12, 200, 2)
    
    style_list = obs.obs_properties_add_list(props, "font_style", "Font Style", 
                                            obs.OBS_COMBO_TYPE_LIST, 
                                            obs.OBS_COMBO_FORMAT_STRING)
    obs.obs_property_list_add_string(style_list, "Regular", "Regular")
    obs.obs_property_list_add_string(style_list, "Bold", "Bold")
    obs.obs_property_list_add_string(style_list, "Italic", "Italic")
    obs.obs_property_list_add_string(style_list, "Bold Italic", "Bold Italic")
    
    obs.obs_properties_add_color(props, "text_color", "Text Color")
    
    # Effects
    obs.obs_properties_add_text(props, "effects_header", "━━━ Text Effects ━━━", obs.OBS_TEXT_INFO)
    
    obs.obs_properties_add_bool(props, "outline_enabled", "Enable Text Outline")
    obs.obs_properties_add_int(props, "outline_size", "Outline Size", 1, 20, 1)
    obs.obs_properties_add_color(props, "outline_color", "Outline Color")
    
    obs.obs_properties_add_bool(props, "shadow_enabled", "Enable Text Shadow")
    obs.obs_properties_add_color(props, "shadow_color", "Shadow Color")
    
    # Info
    obs.obs_properties_add_text(props, "info", "💡 Use the Lyrics Control Panel dock for quick access to controls", obs.OBS_TEXT_INFO)
    
    return props

# Source info structure
source_info = obs.obs_source_info()
source_info.id = "lyrics_overlay_python"
source_info.type = obs.OBS_SOURCE_TYPE_INPUT
source_info.output_flags = obs.OBS_SOURCE_VIDEO | obs.OBS_SOURCE_CUSTOM_DRAW
source_info.get_name = lambda: "Lyrics Overlay (Python)"
source_info.create = source_create
source_info.destroy = source_destroy
source_info.update = source_update
source_info.video_render = source_video_render
source_info.get_width = source_get_width
source_info.get_height = source_get_height
source_info.get_defaults = source_get_defaults
source_info.get_properties = source_get_properties
