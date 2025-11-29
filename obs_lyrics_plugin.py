import obspython as obs
import os
import json
from pathlib import Path

# Global variables
lyrics_data = {}
current_song = None
current_line = 0
source_name = None
lyrics_visible = True

# Plugin properties
props_settings = {
    "background_image": "",
    "lyrics_folder": "",
    "text_h_align": "center",
    "text_v_align": "center",
    "text_width": 800,
    "text_height": 100,
    "text_x_offset": 0,
    "text_y_offset": 0,
    "font_size": 48,
    "font_color": 0xFFFFFFFF,
    "outline_size": 2,
    "outline_color": 0xFF000000,
    "font_family": "Arial"
}

def script_description():
    return """OBS Lyrics Plugin
    
This plugin creates a lyrics display system with:
- Background image support
- Folder-based lyrics management
- Customizable text positioning and styling
- Playback controls in source toolbar"""

def script_properties():
    props = obs.obs_properties_create()
    
    # Background image
    obs.obs_properties_add_path(props, "background_image", "Background Image", 
                                obs.OBS_PATH_FILE, 
                                "Image Files (*.png *.jpg *.jpeg *.gif *.bmp);;All Files (*.*)", 
                                None)
    
    # Lyrics folder
    obs.obs_properties_add_path(props, "lyrics_folder", "Lyrics Folder", 
                                obs.OBS_PATH_DIRECTORY, None, None)
    
    # Text positioning
    h_align = obs.obs_properties_add_list(props, "text_h_align", "Horizontal Alignment", 
                                          obs.OBS_COMBO_TYPE_LIST, obs.OBS_COMBO_FORMAT_STRING)
    obs.obs_property_list_add_string(h_align, "Left", "left")
    obs.obs_property_list_add_string(h_align, "Center", "center")
    obs.obs_property_list_add_string(h_align, "Right", "right")
    
    v_align = obs.obs_properties_add_list(props, "text_v_align", "Vertical Alignment", 
                                          obs.OBS_COMBO_TYPE_LIST, obs.OBS_COMBO_FORMAT_STRING)
    obs.obs_property_list_add_string(v_align, "Top", "top")
    obs.obs_property_list_add_string(v_align, "Center", "center")
    obs.obs_property_list_add_string(v_align, "Bottom", "bottom")
    
    # Text box dimensions
    obs.obs_properties_add_int(props, "text_width", "Text Box Width", 100, 3840, 10)
    obs.obs_properties_add_int(props, "text_height", "Text Box Height", 50, 2160, 10)
    
    # Text box offset
    obs.obs_properties_add_int(props, "text_x_offset", "X Offset", -1920, 1920, 10)
    obs.obs_properties_add_int(props, "text_y_offset", "Y Offset", -1080, 1080, 10)
    
    # Font settings
    obs.obs_properties_add_font(props, "font_family", "Font")
    obs.obs_properties_add_int(props, "font_size", "Font Size", 10, 200, 1)
    obs.obs_properties_add_color(props, "font_color", "Font Color")
    
    # Outline settings
    obs.obs_properties_add_int(props, "outline_size", "Outline Size", 0, 20, 1)
    obs.obs_properties_add_color(props, "outline_color", "Outline Color")
    
    # Source name for updating
    obs.obs_properties_add_text(props, "source_name", "Source Name", obs.OBS_TEXT_DEFAULT)
    
    return props

def script_defaults(settings):
    obs.obs_data_set_default_string(settings, "text_h_align", "center")
    obs.obs_data_set_default_string(settings, "text_v_align", "center")
    obs.obs_data_set_default_int(settings, "text_width", 800)
    obs.obs_data_set_default_int(settings, "text_height", 100)
    obs.obs_data_set_default_int(settings, "text_x_offset", 0)
    obs.obs_data_set_default_int(settings, "text_y_offset", 0)
    obs.obs_data_set_default_int(settings, "font_size", 48)
    obs.obs_data_set_default_int(settings, "font_color", 0xFFFFFFFF)
    obs.obs_data_set_default_int(settings, "outline_size", 2)
    obs.obs_data_set_default_int(settings, "outline_color", 0xFF000000)
    obs.obs_data_set_default_string(settings, "font_family", "Arial")
    obs.obs_data_set_default_string(settings, "source_name", "Lyrics Display")

def script_update(settings):
    global props_settings, source_name, lyrics_data
    
    # Update all settings
    props_settings["background_image"] = obs.obs_data_get_string(settings, "background_image")
    props_settings["lyrics_folder"] = obs.obs_data_get_string(settings, "lyrics_folder")
    props_settings["text_h_align"] = obs.obs_data_get_string(settings, "text_h_align")
    props_settings["text_v_align"] = obs.obs_data_get_string(settings, "text_v_align")
    props_settings["text_width"] = obs.obs_data_get_int(settings, "text_width")
    props_settings["text_height"] = obs.obs_data_get_int(settings, "text_height")
    props_settings["text_x_offset"] = obs.obs_data_get_int(settings, "text_x_offset")
    props_settings["text_y_offset"] = obs.obs_data_get_int(settings, "text_y_offset")
    props_settings["font_size"] = obs.obs_data_get_int(settings, "font_size")
    props_settings["font_color"] = obs.obs_data_get_int(settings, "font_color")
    props_settings["outline_size"] = obs.obs_data_get_int(settings, "outline_size")
    props_settings["outline_color"] = obs.obs_data_get_int(settings, "outline_color")
    props_settings["font_family"] = obs.obs_data_get_string(settings, "font_family")
    source_name = obs.obs_data_get_string(settings, "source_name")
    
    # Load lyrics from folder
    load_lyrics_from_folder()
    
    # Update the display
    update_lyrics_display()

def load_lyrics_from_folder():
    global lyrics_data
    lyrics_data = {}
    
    folder = props_settings["lyrics_folder"]
    if not folder or not os.path.exists(folder):
        return
    
    # Load all .txt files from the folder
    for filename in os.listdir(folder):
        if filename.endswith('.txt'):
            filepath = os.path.join(folder, filename)
            song_name = os.path.splitext(filename)[0]
            
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    lines = [line.strip() for line in f.readlines() if line.strip()]
                    lyrics_data[song_name] = lines
            except Exception as e:
                print(f"Error loading {filename}: {e}")

def calculate_text_position():
    """Calculate the actual position of the text based on alignment and offset"""
    # Get canvas dimensions (assuming 1920x1080, can be made dynamic)
    canvas_width = 1920
    canvas_height = 1080
    
    # Calculate base position based on alignment
    if props_settings["text_h_align"] == "left":
        x = 0
    elif props_settings["text_h_align"] == "center":
        x = (canvas_width - props_settings["text_width"]) // 2
    else:  # right
        x = canvas_width - props_settings["text_width"]
    
    if props_settings["text_v_align"] == "top":
        y = 0
    elif props_settings["text_v_align"] == "center":
        y = (canvas_height - props_settings["text_height"]) // 2
    else:  # bottom
        y = canvas_height - props_settings["text_height"]
    
    # Apply offset
    x += props_settings["text_x_offset"]
    y += props_settings["text_y_offset"]
    
    return x, y

def update_lyrics_display():
    """Update the text source with current lyrics"""
    global current_song, current_line, lyrics_visible
    
    if not source_name:
        return
    
    source = obs.obs_get_source_by_name(source_name)
    if not source:
        return
    
    settings = obs.obs_data_create()
    
    # Set text content
    text = ""
    if lyrics_visible and current_song and current_song in lyrics_data:
        lines = lyrics_data[current_song]
        if 0 <= current_line < len(lines):
            text = lines[current_line]
    
    obs.obs_data_set_string(settings, "text", text)
    
    # Set font
    font_data = obs.obs_data_create()
    obs.obs_data_set_string(font_data, "face", props_settings["font_family"])
    obs.obs_data_set_int(font_data, "size", props_settings["font_size"])
    obs.obs_data_set_int(font_data, "flags", 0)
    obs.obs_data_set_obj(settings, "font", font_data)
    obs.obs_data_release(font_data)
    
    # Set colors
    obs.obs_data_set_int(settings, "color", props_settings["font_color"])
    obs.obs_data_set_int(settings, "outline_color", props_settings["outline_color"])
    obs.obs_data_set_int(settings, "outline_size", props_settings["outline_size"])
    
    # Set alignment
    align_map = {
        "left": 0,
        "center": 1,
        "right": 2
    }
    obs.obs_data_set_int(settings, "align", align_map.get(props_settings["text_h_align"], 1))
    
    # Update source
    obs.obs_source_update(source, settings)
    obs.obs_data_release(settings)
    obs.obs_source_release(source)

# Control functions
def next_line(pressed):
    if not pressed:
        return
    
    global current_line
    if current_song and current_song in lyrics_data:
        lines = lyrics_data[current_song]
        if current_line < len(lines) - 1:
            current_line += 1
            update_lyrics_display()

def previous_line(pressed):
    if not pressed:
        return
    
    global current_line
    if current_line > 0:
        current_line -= 1
        update_lyrics_display()

def toggle_visibility(pressed):
    if not pressed:
        return
    
    global lyrics_visible
    lyrics_visible = not lyrics_visible
    update_lyrics_display()

def select_song(song_name):
    global current_song, current_line
    if song_name in lyrics_data:
        current_song = song_name
        current_line = 0
        update_lyrics_display()

# Hotkey registration
def script_load(settings):
    # Register hotkeys
    obs.obs_hotkey_register_frontend("lyrics_next", "Lyrics: Next Line", next_line)
    obs.obs_hotkey_register_frontend("lyrics_previous", "Lyrics: Previous Line", previous_line)
    obs.obs_hotkey_register_frontend("lyrics_toggle", "Lyrics: Toggle Visibility", toggle_visibility)

def script_save(settings):
    # Save current state
    obs.obs_data_set_string(settings, "current_song", current_song or "")
    obs.obs_data_set_int(settings, "current_line", current_line)
    obs.obs_data_set_bool(settings, "lyrics_visible", lyrics_visible)

# Source creation helper
def create_lyrics_scene():
    """Helper function to create a complete lyrics scene"""
    current_scene = obs.obs_frontend_get_current_scene()
    if not current_scene:
        return
    
    scene = obs.obs_scene_from_source(current_scene)
    
    # Create background image source if specified
    if props_settings["background_image"] and os.path.exists(props_settings["background_image"]):
        image_settings = obs.obs_data_create()
        obs.obs_data_set_string(image_settings, "file", props_settings["background_image"])
        
        image_source = obs.obs_source_create("image_source", "Lyrics Background", image_settings, None)
        obs.obs_scene_add(scene, image_source)
        
        obs.obs_data_release(image_settings)
        obs.obs_source_release(image_source)
    
    # Create text source
    text_settings = obs.obs_data_create()
    obs.obs_data_set_string(text_settings, "text", "")
    
    text_source = obs.obs_source_create("text_gdiplus", source_name or "Lyrics Display", text_settings, None)
    scene_item = obs.obs_scene_add(scene, text_source)
    
    # Position the text
    x, y = calculate_text_position()
    pos = obs.vec2()
    pos.x = x
    pos.y = y
    obs.obs_sceneitem_set_pos(scene_item, pos)
    
    # Set bounds
    bounds = obs.vec2()
    bounds.x = props_settings["text_width"]
    bounds.y = props_settings["text_height"]
    obs.obs_sceneitem_set_bounds(scene_item, bounds)
    obs.obs_sceneitem_set_bounds_type(scene_item, obs.OBS_BOUNDS_SCALE_INNER)
    
    obs.obs_data_release(text_settings)
    obs.obs_source_release(text_source)
    obs.obs_source_release(current_scene)
    
    # Initial update
    update_lyrics_display()
