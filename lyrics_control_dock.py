import obspython as obs
from PyQt5 import QtWidgets, QtCore, QtGui
import sys
import os

# Import main plugin functionality
sys.path.append(os.path.dirname(__file__))
from obs_lyrics_plugin import lyrics_data, current_song, current_line, select_song, next_line, previous_line, toggle_visibility

class LyricsControlDock(QtWidgets.QDockWidget):
    def __init__(self, parent=None):
        super().__init__("Lyrics Control", parent)
        
        # Create main widget
        self.main_widget = QtWidgets.QWidget()
        self.setWidget(self.main_widget)
        
        # Create layout
        layout = QtWidgets.QVBoxLayout(self.main_widget)
        
        # Song selector
        self.song_label = QtWidgets.QLabel("Select Song:")
        layout.addWidget(self.song_label)
        
        self.song_combo = QtWidgets.QComboBox()
        self.song_combo.currentTextChanged.connect(self.on_song_changed)
        layout.addWidget(self.song_combo)
        
        # Current line display
        self.line_label = QtWidgets.QLabel("Current Line: 0 / 0")
        layout.addWidget(self.line_label)
        
        # Control buttons
        button_layout = QtWidgets.QHBoxLayout()
        
        self.prev_button = QtWidgets.QPushButton("◀ Previous")
        self.prev_button.clicked.connect(lambda: previous_line(True))
        button_layout.addWidget(self.prev_button)
        
        self.next_button = QtWidgets.QPushButton("Next ▶")
        self.next_button.clicked.connect(lambda: next_line(True))
        button_layout.addWidget(self.next_button)
        
        layout.addLayout(button_layout)
        
        # Visibility toggle
        self.visibility_button = QtWidgets.QPushButton("Hide Lyrics")
        self.visibility_button.setCheckable(True)
        self.visibility_button.clicked.connect(self.on_visibility_toggle)
        layout.addWidget(self.visibility_button)
        
        # Refresh button
        self.refresh_button = QtWidgets.QPushButton("Refresh Songs")
        self.refresh_button.clicked.connect(self.refresh_songs)
        layout.addWidget(self.refresh_button)
        
        # Add stretch
        layout.addStretch()
        
        # Set minimum size
        self.setMinimumWidth(250)
        
        # Initial refresh
        self.refresh_songs()
        
        # Timer for updating display
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update_display)
        self.timer.start(100)  # Update every 100ms
    
    def refresh_songs(self):
        """Refresh the song list from lyrics_data"""
        self.song_combo.clear()
        self.song_combo.addItem("-- Select Song --")
        
        if lyrics_data:
            for song_name in sorted(lyrics_data.keys()):
                self.song_combo.addItem(song_name)
    
    def on_song_changed(self, song_name):
        """Handle song selection"""
        if song_name and song_name != "-- Select Song --":
            select_song(song_name)
            self.update_display()
    
    def on_visibility_toggle(self):
        """Handle visibility toggle"""
        toggle_visibility(True)
        if self.visibility_button.isChecked():
            self.visibility_button.setText("Show Lyrics")
        else:
            self.visibility_button.setText("Hide Lyrics")
    
    def update_display(self):
        """Update the current line display"""
        if current_song and current_song in lyrics_data:
            total_lines = len(lyrics_data[current_song])
            self.line_label.setText(f"Current Line: {current_line + 1} / {total_lines}")
            
            # Update button states
            self.prev_button.setEnabled(current_line > 0)
            self.next_button.setEnabled(current_line < total_lines - 1)
        else:
            self.line_label.setText("Current Line: 0 / 0")
            self.prev_button.setEnabled(False)
            self.next_button.setEnabled(False)

# Global dock reference
lyrics_dock = None

def script_load(settings):
    global lyrics_dock
    
    # Create and add dock to OBS
    main_window = obs.obs_frontend_get_main_window()
    if main_window:
        lyrics_dock = LyricsControlDock(main_window)
        obs.obs_frontend_add_dock(lyrics_dock)

def script_unload():
    global lyrics_dock
    if lyrics_dock:
        lyrics_dock.close()
        lyrics_dock = None

def script_description():
    return "Lyrics Control Dock - Provides a control interface for the OBS Lyrics Plugin"
