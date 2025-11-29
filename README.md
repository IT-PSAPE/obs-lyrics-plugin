# OBS Lyrics Plugin

A powerful OBS Studio plugin for displaying lyrics and presentations with full customization options. Available in both Lua (no dependencies) and Python (with dock UI) versions! This source allows you to display lyrics from text files over a background image, with advanced control over navigation and text styling.

## Features

- **Folder-based Song Management**: Load all songs from a folder - each `.txt` file is a separate song
- **Line-by-Line Display**: Each line in the text file is displayed as a separate slide
- **Background Images**: Display custom background images behind your lyrics
- **Full Text Customization**:
    - **Font Selection**: Choose font face, size, and style
    - **Text Color**: Full color picker for text
    - **Text Outline/Border**: Enable outline with customizable color and thickness
    - **Drop Shadow**: Add shadows with adjustable opacity, offset, and color
- **Text Box System**: 
    - Adjustable width and height
    - Alignment-based positioning (no manual offsets)
    - Visual bounds indicator for easy setup
- **Playback Controls**: 
    - Navigate between lyrics (Previous/Next Lyric)
    - Stop/Clear current display
    - Python version includes dock widget

## Choose Your Version

### Lua Version (Recommended for simplicity)
- **Location**: `lua_version/` folder
- **Pros**: No dependencies, works immediately
- **Features**: All core features, external control script

### Python Version (Recommended for dock UI)
- **Location**: `python_version/` folder
- **Pros**: Dock widget UI next to Properties panel
- **Requires**: Python 3.6+ with PyQt5

## Installation

### For Lua Version:
1. Go to Tools → Scripts
2. Add `lua_version/lyrics_plugin.lua`
3. (Optional) Add `lua_version/lyrics_controls.lua`

### For Python Version:
1. Ensure OBS has Python support
2. Go to Tools → Scripts
3. Add `python_version/lyrics_plugin_main.py`
4. The dock widget appears automatically

## Usage

1. **Add the Source**:
    - In OBS, click the `+` button in the Sources panel.
    - Select `Lyrics Overlay` from the list.
    - Name your source and click OK.

2. **Configure Properties**:
    - **Background Image**: (Optional) Select an image to display behind the lyrics.
    - **Songs Folder**: Select a folder containing your `.txt` song files.
    - **Current Song**: Choose which song to display from the dropdown.
    - **Text Box**: Set width and height of the text area.
    - **Text Position**: Choose alignment (left/center/right, top/center/bottom).
    - **Text Appearance**: Customize font, size, style, and color.
    - **Text Effects**: Enable and configure outline and shadow effects.

3. **Control Playback**:
    - **In Properties**: Use the quick control buttons
        - **◀ Previous Lyric**: Go to the previous line
        - **■ Stop/Clear**: Clear the current display
        - **▶ Next Lyric**: Advance to the next line
    - **External Control Panel**: Use the separate control script
        - Song selection dropdown
        - Playback control buttons
        - Configurable hotkeys

## File Format

- Create a folder for your songs/presentations
- Each `.txt` file in the folder is treated as a separate song or presentation
- Each line in the text file is displayed as a separate slide
- Empty lines will display as blank slides
- UTF-8 encoding is supported for international characters

### Example File Structure:
```
songs_folder/
├── Amazing_Grace.txt
├── Test_Song.txt
└── Demo_Presentation.txt
```

### Example Song File (Amazing_Grace.txt):
```
Amazing Grace, how sweet the sound
That saved a wretch like me
I once was lost but now am found
Was blind, but now I see
```

## Troubleshooting

- **Text not appearing?** Ensure "Stop/Clear" wasn't clicked. Try clicking "Next Lyric" to advance to the first line.
- **Songs not showing in dropdown?** Make sure you've selected a valid folder containing .txt files, then click away and back to refresh.
- **Plugin not loading?** Check the OBS Script Log (Tools → Scripts → Script Log button) for any Lua errors.
