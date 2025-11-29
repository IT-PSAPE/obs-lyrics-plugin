# OBS Lyrics Plugin

A comprehensive lyrics display system for OBS Studio that allows you to display song lyrics with customizable backgrounds and text styling.

## Features

- **Background Image Support**: Load any image as a background for your lyrics
- **Folder-based Lyrics Management**: Organize your songs as individual .txt files in a folder
- **Customizable Text Positioning**: 
  - Horizontal alignment (left, center, right)
  - Vertical alignment (top, center, bottom)
  - Custom width and height for text box
  - X/Y offset for fine-tuning position
- **Text Styling Options**:
  - Font family selection
  - Font size adjustment
  - Text color customization
  - Outline size and color
- **Playback Controls**:
  - Next/Previous line navigation
  - Hide/Show lyrics toggle
  - Song selection
- **Multiple Control Interfaces**:
  - Control dock window
  - Source toolbar integration
  - Hotkey support

## Installation

1. **Prerequisites**:
   - OBS Studio (version 27.0 or higher)
   - Python 3.6+ (must match OBS Python version)
   - PyQt5 (for control dock)

2. **Install the Plugin**:
   - Copy all `.py` files to your OBS scripts folder
   - In OBS, go to Tools → Scripts
   - Click the "+" button and add `obs_lyrics_plugin.py`
   - Optionally add `lyrics_control_dock.py` for the control panel
   - Optionally add `lyrics_source_toolbar.py` for toolbar integration

## Usage

### Initial Setup

1. **Create a Lyrics Folder Structure**:
   ```
   lyrics/
   ├── Song1.txt
   ├── Song2.txt
   └── Song3.txt
   ```
   
   Each .txt file should contain lyrics with one line per slide:
   ```
   First line of the song
   Second line of the song
   Third line of the song
   ```

2. **Configure the Plugin**:
   - In OBS Scripts, select the lyrics plugin
   - Set the following properties:
     - **Background Image**: Select your background image file
     - **Lyrics Folder**: Select the folder containing your .txt files
     - **Source Name**: Name for the text source (default: "Lyrics Display")

3. **Create a Scene**:
   - Create a new scene in OBS
   - The plugin will automatically create the background and text sources
   - Or manually add a Text (GDI+) source with the name you specified

### Text Positioning

- **Horizontal Alignment**: Choose left, center, or right
- **Vertical Alignment**: Choose top, center, or bottom
- **Text Box Width/Height**: Set the dimensions of the text area
- **X/Y Offset**: Fine-tune the position with pixel offsets

### Text Styling

- **Font**: Select from available system fonts
- **Font Size**: Adjust text size (10-200)
- **Font Color**: Choose text color
- **Outline Size**: Set outline thickness (0-20)
- **Outline Color**: Choose outline color

### Controls

#### Control Dock
If you've added `lyrics_control_dock.py`:
- A "Lyrics Control" dock will appear in OBS
- Select songs from the dropdown
- Use Previous/Next buttons to navigate lines
- Toggle visibility with Hide/Show button
- Refresh song list after adding new files

#### Hotkeys
The plugin registers these hotkeys (set in OBS Settings → Hotkeys):
- **Lyrics: Next Line**: Move to next lyric line
- **Lyrics: Previous Line**: Move to previous line
- **Lyrics: Toggle Visibility**: Show/hide lyrics

#### Source Toolbar
If you've added `lyrics_source_toolbar.py`:
- Right-click on the lyrics text source
- Access lyrics controls from the context menu

## File Format

### Lyrics Files (.txt)
- UTF-8 encoded text files
- One line per slide
- Empty lines are skipped
- File name becomes the song name (without .txt extension)

Example `Amazing Grace.txt`:
```
Amazing grace, how sweet the sound
That saved a wretch like me
I once was lost, but now am found
Was blind, but now I see
```

## Tips

1. **Background Images**: Use high-resolution images that match your stream resolution
2. **Text Contrast**: Use outline to ensure text is readable on any background
3. **Font Selection**: Choose clear, readable fonts for streaming
4. **Organization**: Name your files clearly (e.g., "01 - Song Title.txt" for ordering)
5. **Testing**: Test your setup before going live to ensure smooth operation

## Troubleshooting

### Text Not Showing
- Ensure the source name in settings matches your text source
- Check that the lyrics folder path is correct
- Verify .txt files are properly formatted

### Control Dock Not Appearing
- Make sure PyQt5 is installed
- Check OBS logs for any Python errors
- Try restarting OBS after adding the script

### Hotkeys Not Working
- Set up hotkeys in OBS Settings → Hotkeys
- Look for "Lyrics: Next Line", etc.
- Assign your preferred key combinations

## Advanced Usage

### Multiple Lyrics Displays
You can create multiple instances by:
1. Duplicating the script files with different names
2. Using different source names for each instance
3. Managing separate lyrics folders

### Custom Styling
For advanced styling beyond the plugin options:
1. Create the text source manually
2. Apply OBS filters (color correction, etc.)
3. Use the plugin just for content management

## License

This plugin is provided as-is for use with OBS Studio. Feel free to modify and distribute according to your needs.

## Support

For issues or feature requests, please check:
- OBS logs (Help → Log Files → View Current Log)
- Python script output in the Scripts window
- Ensure all dependencies are properly installed
