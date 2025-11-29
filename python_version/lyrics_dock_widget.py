from PyQt5 import QtWidgets, QtCore, QtGui
import obspython as obs
import os

class LyricsDockWidget(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Lyrics Control Panel")
        self.setup_ui()
        self.current_source = None
        
        # Update timer
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update_status)
        self.timer.start(500)
        
    def setup_ui(self):
        layout = QtWidgets.QVBoxLayout()
        self.setLayout(layout)
        
        # Title
        title = QtWidgets.QLabel("Lyrics Control Panel")
        title.setAlignment(QtCore.Qt.AlignCenter)
        title.setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;")
        layout.addWidget(title)
        
        # Source selector
        source_group = QtWidgets.QGroupBox("Source Selection")
        source_layout = QtWidgets.QVBoxLayout()
        
        self.source_combo = QtWidgets.QComboBox()
        self.source_combo.currentTextChanged.connect(self.on_source_changed)
        source_layout.addWidget(self.source_combo)
        
        self.refresh_btn = QtWidgets.QPushButton("🔄 Refresh Sources")
        self.refresh_btn.clicked.connect(self.refresh_sources)
        source_layout.addWidget(self.refresh_btn)
        
        source_group.setLayout(source_layout)
        layout.addWidget(source_group)
        
        # Song controls
        song_group = QtWidgets.QGroupBox("Song Selection")
        song_layout = QtWidgets.QVBoxLayout()
        
        self.song_combo = QtWidgets.QComboBox()
        self.song_combo.currentTextChanged.connect(self.on_song_changed)
        song_layout.addWidget(self.song_combo)
        
        song_group.setLayout(song_layout)
        layout.addWidget(song_group)
        
        # Status display
        status_group = QtWidgets.QGroupBox("Current Status")
        status_layout = QtWidgets.QVBoxLayout()
        
        self.song_label = QtWidgets.QLabel("Song: None")
        self.lyric_label = QtWidgets.QLabel("Lyric: 0/0")
        self.preview_label = QtWidgets.QLabel("Preview: No lyric loaded")
        self.preview_label.setWordWrap(True)
        self.preview_label.setStyleSheet("padding: 10px; background-color: #f0f0f0; border-radius: 5px;")
        
        status_layout.addWidget(self.song_label)
        status_layout.addWidget(self.lyric_label)
        status_layout.addWidget(self.preview_label)
        
        status_group.setLayout(status_layout)
        layout.addWidget(status_group)
        
        # Playback controls
        controls_group = QtWidgets.QGroupBox("Playback Controls")
        controls_layout = QtWidgets.QHBoxLayout()
        
        self.prev_btn = QtWidgets.QPushButton("◀ Previous")
        self.stop_btn = QtWidgets.QPushButton("■ Stop/Clear")
        self.next_btn = QtWidgets.QPushButton("Next ▶")
        
        self.prev_btn.clicked.connect(self.prev_lyric)
        self.stop_btn.clicked.connect(self.stop_lyrics)
        self.next_btn.clicked.connect(self.next_lyric)
        
        controls_layout.addWidget(self.prev_btn)
        controls_layout.addWidget(self.stop_btn)
        controls_layout.addWidget(self.next_btn)
        
        controls_group.setLayout(controls_layout)
        layout.addWidget(controls_group)
        
        layout.addStretch()
        
        # Apply styling
        self.setStyleSheet("""
            QWidget {
                font-family: Arial, sans-serif;
            }
            QGroupBox {
                font-weight: bold;
                border: 2px solid #cccccc;
                border-radius: 5px;
                margin-top: 10px;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
            QPushButton {
                padding: 8px;
                border-radius: 3px;
                background-color: #4a90e2;
                color: white;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #357abd;
            }
            QPushButton:pressed {
                background-color: #2968a3;
            }
            QComboBox {
                padding: 5px;
                border-radius: 3px;
            }
        """)
        
    def refresh_sources(self):
        self.source_combo.clear()
        self.source_combo.addItem("None")
        
        sources = obs.obs_enum_sources()
        if sources:
            for source in sources:
                source_id = obs.obs_source_get_id(source)
                if source_id == "lyrics_overlay_python":
                    name = obs.obs_source_get_name(source)
                    self.source_combo.addItem(name)
            obs.source_list_release(sources)
            
    def on_source_changed(self, source_name):
        self.current_source = source_name if source_name != "None" else None
        self.update_source_info()
        
    def update_source_info(self):
        from lyrics_plugin import lyrics_instances
        
        self.song_combo.clear()
        
        if self.current_source and self.current_source in lyrics_instances:
            instance = lyrics_instances[self.current_source]
            
            # Update song list
            for song in instance.available_songs:
                self.song_combo.addItem(song)
                
            # Set current song
            if instance.current_song_file:
                song_name = os.path.basename(instance.current_song_file)
                index = self.song_combo.findText(song_name)
                if index >= 0:
                    self.song_combo.setCurrentIndex(index)
                    
    def on_song_changed(self, song_name):
        from lyrics_plugin import lyrics_instances
        
        if not song_name or not self.current_source:
            return
            
        if self.current_source in lyrics_instances:
            instance = lyrics_instances[self.current_source]
            song_path = os.path.join(instance.songs_folder, song_name)
            
            # Update source settings
            source = obs.obs_get_source_by_name(self.current_source)
            if source:
                settings = obs.obs_source_get_settings(source)
                obs.obs_data_set_string(settings, "song_selection", song_path)
                obs.obs_source_update(source, settings)
                obs.obs_data_release(settings)
                obs.obs_source_release(source)
                
    def update_status(self):
        from lyrics_plugin import lyrics_instances
        
        if self.current_source and self.current_source in lyrics_instances:
            instance = lyrics_instances[self.current_source]
            info = instance.get_info()
            
            self.song_label.setText(f"Song: {info['song_name']}")
            self.lyric_label.setText(f"Lyric: {info['current_line']}/{info['total_lines']}")
            
            preview = info['current_lyric'] or "No lyric displayed"
            if len(preview) > 100:
                preview = preview[:100] + "..."
            self.preview_label.setText(f"Preview: {preview}")
            
    def next_lyric(self):
        from lyrics_plugin import lyrics_instances
        
        if self.current_source and self.current_source in lyrics_instances:
            lyrics_instances[self.current_source].next_lyric()
            
    def prev_lyric(self):
        from lyrics_plugin import lyrics_instances
        
        if self.current_source and self.current_source in lyrics_instances:
            lyrics_instances[self.current_source].prev_lyric()
            
    def stop_lyrics(self):
        from lyrics_plugin import lyrics_instances
        
        if self.current_source and self.current_source in lyrics_instances:
            lyrics_instances[self.current_source].stop_lyrics()
