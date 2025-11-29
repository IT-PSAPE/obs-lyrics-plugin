import obspython as obs
import os
from PyQt5 import QtWidgets, QtCore, QtGui

# Global variables
lyrics_instances = {}
dock_widget = None

# Default values that match what's displayed
DEFAULTS = {
    'font_size': 48,
    'font_face': 'Arial',
    'font_style': 'Regular',
    'text_color': 0xFFFFFFFF,
    'text_width': 800,
    'text_height': 600,
    'h_align': 'center',
    'v_align': 'center',
    'outline_enabled': False,
    'outline_size': 2,
    'outline_color': 0xFF000000,
    'shadow_enabled': False,
    'shadow_color': 0xFF000000,
    'show_text_bounds': False,
    'text_bounds_color': 0x80FF0000  # Semi-transparent red
}

class LyricsSource:
    def __init__(self, source, settings):
        self.source = source
        self.settings = settings
        self.source_name = obs.obs_source_get_name(source)
        
        # File and folder settings
        self.image_path = ""
        self.songs_folder = ""
        self.current_song_file = ""
        self.lyrics_lines = []
        self.current_line_index = 0
        self.is_visible = True
        
        # Sources
        self.text_source = None
        self.image_source = None
        self.bounds_source = None
        
        # Dimensions
        self.width = 1920
        self.height = 1080
        
        # Song management
        self.available_songs = []
        self.current_song_index = -1
        
        # Register instance
        lyrics_instances[self.source_name] = self
        
        # Initialize text source with defaults
        text_settings = obs.obs_data_create()
        self.apply_defaults(text_settings)
        obs.obs_data_set_string(text_settings, "text", "Click Next Lyric to begin")
        self.text_source = obs.obs_source_create_private("text_ft2_source", f"lyrics_text_{id(self)}", text_settings)
        obs.obs_data_release(text_settings)
        
        # Initialize image source
        img_settings = obs.obs_data_create()
        self.image_source = obs.obs_source_create_private("image_source", f"lyrics_bg_{id(self)}", img_settings)
        obs.obs_data_release(img_settings)
        
        # Initialize bounds visualization
        bounds_settings = obs.obs_data_create()
        obs.obs_data_set_int(bounds_settings, "width", DEFAULTS['text_width'])
        obs.obs_data_set_int(bounds_settings, "height", DEFAULTS['text_height'])
        obs.obs_data_set_int(bounds_settings, "color", DEFAULTS['text_bounds_color'])
        self.bounds_source = obs.obs_source_create_private("color_source", f"lyrics_bounds_{id(self)}", bounds_settings)
        obs.obs_data_release(bounds_settings)
        
    def apply_defaults(self, settings):
        """Apply default values to text settings"""
        font_obj = obs.obs_data_create()
        obs.obs_data_set_string(font_obj, "face", DEFAULTS['font_face'])
        obs.obs_data_set_int(font_obj, "size", DEFAULTS['font_size'])
        obs.obs_data_set_string(font_obj, "style", DEFAULTS['font_style'])
        obs.obs_data_set_obj(settings, "font", font_obj)
        obs.obs_data_release(font_obj)
        
        obs.obs_data_set_int(settings, "color1", DEFAULTS['text_color'])
        obs.obs_data_set_int(settings, "color2", DEFAULTS['text_color'])
        obs.obs_data_set_string(settings, "align", DEFAULTS['h_align'])
        obs.obs_data_set_string(settings, "valign", DEFAULTS['v_align'])
        obs.obs_data_set_bool(settings, "word_wrap", True)
        obs.obs_data_set_int(settings, "custom_width", DEFAULTS['text_width'])
        
    def update(self, settings):
        self.settings = settings
        self.image_path = obs.obs_data_get_string(settings, "background_image")
        self.songs_folder = obs.obs_data_get_string(settings, "songs_folder")
        new_song_file = obs.obs_data_get_string(settings, "song_selection")
        
        # Check if song changed
        song_changed = (new_song_file != self.current_song_file)
        self.current_song_file = new_song_file
        
        # Update background image
        if self.image_path and self.image_source:
            img_settings = obs.obs_data_create()
            obs.obs_data_set_string(img_settings, "file", self.image_path)
            obs.obs_source_update(self.image_source, img_settings)
            obs.obs_data_release(img_settings)
            
        # Update bounds visualization
        show_bounds = obs.obs_data_get_bool(settings, "show_text_bounds")
        if self.bounds_source:
            bounds_settings = obs.obs_data_create()
            obs.obs_data_set_int(bounds_settings, "width", obs.obs_data_get_int(settings, "text_width"))
            obs.obs_data_set_int(bounds_settings, "height", obs.obs_data_get_int(settings, "text_height"))
            obs.obs_data_set_int(bounds_settings, "color", obs.obs_data_get_int(settings, "text_bounds_color"))
            obs.obs_source_update(self.bounds_source, bounds_settings)
            obs.obs_data_release(bounds_settings)
        
        # Load available songs
        self.load_available_songs()
        
        # Update text appearance
        self.update_text_appearance()
        
        # Load lyrics if song changed
        if song_changed and self.current_song_file:
            self.load_lyrics(self.current_song_file)
            
        # Update dock widget if exists
        global dock_widget
        if dock_widget:
            dock_widget.update_source_info()
            
    def update_text_appearance(self):
        if not self.text_source:
            return
            
        text_settings = obs.obs_data_create()
        
        # Font settings
        font_size = obs.obs_data_get_int(self.settings, "font_size") or DEFAULTS['font_size']
        font_face = obs.obs_data_get_string(self.settings, "font_face") or DEFAULTS['font_face']
        font_style = obs.obs_data_get_string(self.settings, "font_style") or DEFAULTS['font_style']
        
        font_obj = obs.obs_data_create()
        obs.obs_data_set_string(font_obj, "face", font_face)
        obs.obs_data_set_int(font_obj, "size", font_size)
        obs.obs_data_set_string(font_obj, "style", font_style)
        obs.obs_data_set_obj(text_settings, "font", font_obj)
        
        # Set current text
        current_text = self.get_current_lyric()
        if not current_text and self.current_line_index == 0 and not self.lyrics_lines:
            current_text = "No lyrics loaded. Select a song and click Next Lyric."
        obs.obs_data_set_string(text_settings, "text", current_text)
        
        # Alignment
        h_align = obs.obs_data_get_string(self.settings, "h_align") or DEFAULTS['h_align']
        v_align = obs.obs_data_get_string(self.settings, "v_align") or DEFAULTS['v_align']
        obs.obs_data_set_string(text_settings, "align", h_align)
        obs.obs_data_set_string(text_settings, "valign", v_align)
        
        # Color
        text_color = obs.obs_data_get_int(self.settings, "text_color") or DEFAULTS['text_color']
        obs.obs_data_set_int(text_settings, "color1", text_color)
        obs.obs_data_set_int(text_settings, "color2", text_color)
        
        # Outline
        outline_enabled = obs.obs_data_get_bool(self.settings, "outline_enabled")
        obs.obs_data_set_bool(text_settings, "outline", outline_enabled)
        if outline_enabled:
            outline_size = obs.obs_data_get_int(self.settings, "outline_size") or DEFAULTS['outline_size']
            outline_color = obs.obs_data_get_int(self.settings, "outline_color") or DEFAULTS['outline_color']
            obs.obs_data_set_int(text_settings, "outline_size", outline_size)
            obs.obs_data_set_int(text_settings, "outline_color", outline_color)
            
        # Shadow
        shadow_enabled = obs.obs_data_get_bool(self.settings, "shadow_enabled")
        obs.obs_data_set_bool(text_settings, "drop_shadow", shadow_enabled)
        if shadow_enabled:
            shadow_color = obs.obs_data_get_int(self.settings, "shadow_color") or DEFAULTS['shadow_color']
            obs.obs_data_set_int(text_settings, "shadow_color", shadow_color)
            
        # Word wrap
        obs.obs_data_set_bool(text_settings, "word_wrap", True)
        text_width = obs.obs_data_get_int(self.settings, "text_width") or DEFAULTS['text_width']
        obs.obs_data_set_int(text_settings, "custom_width", text_width)
        
        obs.obs_source_update(self.text_source, text_settings)
        obs.obs_data_release(text_settings)
        obs.obs_data_release(font_obj)
        
    def load_available_songs(self):
        self.available_songs = []
        if self.songs_folder and os.path.exists(self.songs_folder):
            files = [f for f in os.listdir(self.songs_folder) if f.endswith('.txt')]
            self.available_songs = sorted(files)
            
            if self.current_song_file:
                song_name = os.path.basename(self.current_song_file)
                try:
                    self.current_song_index = self.available_songs.index(song_name)
                except ValueError:
                    self.current_song_index = -1
                    
    def load_lyrics(self, file_path):
        self.lyrics_lines = []
        if os.path.exists(file_path):
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    self.lyrics_lines = [line.rstrip() for line in f.readlines()]
            except Exception as e:
                print(f"Error loading lyrics: {e}")
        self.current_line_index = 0
        self.update_text_display()
        
    def get_current_lyric(self):
        if not self.is_visible:
            return ""
        if 0 < self.current_line_index <= len(self.lyrics_lines):
            return self.lyrics_lines[self.current_line_index - 1]
        return ""
        
    def video_render(self, effect):
        if not self.is_visible:
            return
            
        # Draw background
        if self.image_source:
            obs.obs_source_video_render(self.image_source)
            
        # Get dimensions and alignment
        text_width = obs.obs_data_get_int(self.settings, "text_width") or DEFAULTS['text_width']
        text_height = obs.obs_data_get_int(self.settings, "text_height") or DEFAULTS['text_height']
        h_align = obs.obs_data_get_string(self.settings, "h_align") or DEFAULTS['h_align']
        v_align = obs.obs_data_get_string(self.settings, "v_align") or DEFAULTS['v_align']
        
        # Calculate position
        x_offset = 0
        y_offset = 0
        source_width = self.get_width()
        source_height = self.get_height()
        
        if h_align == "center":
            x_offset = (source_width - text_width) / 2
        elif h_align == "right":
            x_offset = source_width - text_width
            
        if v_align == "center":
            y_offset = (source_height - text_height) / 2
        elif v_align == "bottom":
            y_offset = source_height - text_height
            
        # Draw bounds if enabled
        show_bounds = obs.obs_data_get_bool(self.settings, "show_text_bounds")
        if show_bounds and self.bounds_source:
            obs.gs_matrix_push()
            obs.gs_matrix_translate3f(x_offset, y_offset, 0.0)
            obs.obs_source_video_render(self.bounds_source)
            obs.gs_matrix_pop()
            
        # Draw text
        if self.text_source:
            obs.gs_matrix_push()
            obs.gs_matrix_translate3f(x_offset, y_offset, 0.0)
            
            # Clip to bounds
            obs.gs_viewport_push()
            obs.gs_set_viewport(int(x_offset), int(y_offset), int(text_width), int(text_height))
            
            obs.obs_source_video_render(self.text_source)
            
            obs.gs_viewport_pop()
            obs.gs_matrix_pop()
            
    def next_lyric(self):
        if self.current_line_index < len(self.lyrics_lines):
            self.current_line_index += 1
            self.update_text_display()
            return True
        return False
        
    def prev_lyric(self):
        if self.current_line_index > 0:
            self.current_line_index -= 1
            self.update_text_display()
            return True
        return False
        
    def stop_lyrics(self):
        self.current_line_index = 0
        self.update_text_display()
        
    def update_text_display(self):
        if self.text_source:
            settings = obs.obs_data_create()
            text = self.get_current_lyric()
            if not text and self.current_line_index == 0 and self.lyrics_lines:
                text = "[Ready - Click Next to start]"
            elif not text and not self.lyrics_lines:
                text = "[No song loaded]"
            obs.obs_data_set_string(settings, "text", text)
            obs.obs_source_update(self.text_source, settings)
            obs.obs_data_release(settings)
            
    def get_width(self):
        if self.image_source:
            w = obs.obs_source_get_width(self.image_source)
            if w > 0:
                return w
        return self.width
        
    def get_height(self):
        if self.image_source:
            h = obs.obs_source_get_height(self.image_source)
            if h > 0:
                return h
        return self.height
        
    def get_info(self):
        song_name = os.path.basename(self.current_song_file) if self.current_song_file else "No song"
        return {
            'song_name': song_name,
            'current_line': self.current_line_index,
            'total_lines': len(self.lyrics_lines),
            'current_lyric': self.get_current_lyric()
        }
        
    def destroy(self):
        if self.source_name in lyrics_instances:
            del lyrics_instances[self.source_name]
        if self.text_source:
            obs.obs_source_release(self.text_source)
        if self.image_source:
            obs.obs_source_release(self.image_source)
        if self.bounds_source:
            obs.obs_source_release(self.bounds_source)
