# OBS Lyrics Plugin - Quick Start Guide

## Installation (5 minutes)

1. **Open OBS Studio**
2. Go to **Tools → Scripts**
3. Click the **"+"** button
4. Select `obs_lyrics_all_in_one.py` (the easiest option)
5. Click **Open**

## Setup (2 minutes)

1. In the Scripts window, select the lyrics plugin
2. Set these essential properties:
   - **Lyrics Folder**: Browse to `sample_lyrics` folder (included)
   - **Background Image**: (Optional) Select any image
   - **Text Source Name**: Leave as "Lyrics Display"
3. Click **"Create Lyrics Scene"** button

## Usage

### Quick Test
1. In the script properties, find **"Current Song"** dropdown
2. Select "Test Song" 
3. Use these buttons to control:
   - **Next Line ▶**: Move forward
   - **◀ Previous Line**: Move backward
   - **Toggle Visibility**: Hide/show lyrics

### Keyboard Shortcuts
1. Go to **Settings → Hotkeys**
2. Search for "Lyrics"
3. Set keys for:
   - Lyrics: Next Line (e.g., Page Down)
   - Lyrics: Previous Line (e.g., Page Up)
   - Lyrics: Toggle Visibility (e.g., H)

### Adding Your Own Songs
1. Create `.txt` files in the lyrics folder
2. One line per slide
3. File name = Song name
4. Example:
   ```
   First line of lyrics
   Second line of lyrics
   Third line of lyrics
   ```

## Tips
- Start with the included sample songs
- Adjust text position/size in script properties
- Use outline for better readability
- Test everything before going live!

## Need More Control?
- Use `obs_lyrics_plugin.py` + `lyrics_control_dock.py` for advanced features
- See README.md for complete documentation
