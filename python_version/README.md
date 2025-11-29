# Lyrics Overlay Plugin - Python Version

This is the Python version of the Lyrics Overlay plugin with a dock widget UI for easy control.

## Features

- **Dock Widget UI**: Control panel that docks next to Properties/Filters
- **All Lua Features**: Same functionality as the Lua version
- **Live Status Updates**: See current song and lyric in the dock
- **Quick Controls**: Change songs and navigate lyrics without opening properties

## Requirements

- OBS Studio with Python support
- Python 3.6+
- PyQt5 (usually comes with OBS Python)

## Installation

1. Ensure OBS has Python support enabled
2. Open OBS Studio
3. Go to Tools → Scripts
4. Click the + button
5. Add `lyrics_plugin_main.py`

## Dock Widget

The Python version includes a dock widget that appears next to your Properties panel:

- **Source Selection**: Choose which Lyrics Overlay source to control
- **Song Selection**: Dropdown to quickly change songs
- **Status Display**: Shows current song, lyric number, and preview
- **Playback Controls**: Previous/Stop/Next buttons

## Usage

1. Add a new source: Sources → + → Lyrics Overlay (Python)
2. The dock widget will appear automatically
3. Use either the properties panel or dock widget to control playback

## Python Installation Help

If OBS can't find Python:
- **Windows**: Install Python 3.6+ and ensure it matches OBS architecture (64-bit)
- **macOS**: OBS typically uses system Python
- **Linux**: Install python3 and python3-pyqt5 packages

Check Tools → Scripts → Python Settings to verify Python is detected.
