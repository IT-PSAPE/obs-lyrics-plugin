# Lyrics Overlay Plugin - Lua Version

This is the Lua version of the Lyrics Overlay plugin. It works out of the box with OBS Studio without any additional dependencies.

## Features

- **Background Image Support**: Display lyrics over custom background images
- **Text Box with Alignment**: Position text using alignment (left/center/right, top/center/bottom)
- **Adjustable Text Box**: Set custom width and height for the text area
- **Text Box Outline**: Toggle a visual outline to see the text box boundaries while positioning
- **Text Effects**: Add outline and shadow to text for better visibility
- **Proper Default Values**: All default values match what's displayed (48pt font, white text, etc.)

## Installation

1. Open OBS Studio
2. Go to Tools → Scripts
3. Click the + button
4. Add `lyrics_plugin.lua`
5. (Optional) Add `lyrics_controls.lua` for external controls

## Usage

1. Add a new source: Sources → + → Lyrics Overlay
2. Configure:
   - Select a background image (optional)
   - Choose a songs folder containing .txt files
   - Adjust text box size and position
   - Customize text appearance
3. Use playback controls to navigate through lyrics

## Text Positioning

Instead of manual X/Y offsets, this version uses alignment-based positioning:
- **Horizontal**: Left, Center, or Right
- **Vertical**: Top, Center, or Bottom

The text box will automatically position itself based on these alignments.

## File Format

- Each .txt file in your songs folder is a separate song
- Each line in the file is displayed as a separate slide
- Empty lines create blank slides
