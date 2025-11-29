# Alternative UI Solutions for OBS Lyrics Plugin

Since OBS Lua doesn't support creating dock widgets and Python requires additional setup, here are alternative approaches to get a control panel UI:

## Option 1: Browser Source Window (Recommended)

This is the simplest approach that works with just OBS, no additional dependencies.

### Setup:
1. Create a new Scene called "Lyrics Control Panel"
2. Add a Browser Source to this scene
3. Configure the Browser Source:
   - **Local File**: Check this option
   - **URL**: `file:///[full path to]/lua_version/lyrics_web_control.html`
   - **Width**: 400
   - **Height**: 600
4. Right-click the Browser Source → **Windowed Projector (Source)**
5. Position this window next to your OBS window

### Benefits:
- No additional dependencies
- Works on all platforms
- Can be styled with CSS
- Updates in real-time

### Limitations:
- Not a true dock (separate window)
- Requires manual setup

## Option 2: OBS WebSocket + External App

You could use OBS WebSocket plugin (built into OBS 28+) with an external control app:

1. Enable WebSocket Server in OBS (Tools → WebSocket Server Settings)
2. Use any WebSocket client to control the plugin
3. Could be a simple HTML file opened in browser or a standalone app

## Option 3: Stream Deck / Touch Portal Integration

If you have a Stream Deck or use Touch Portal:

1. Create buttons that trigger hotkeys
2. Configure hotkeys in OBS for Previous/Next/Stop
3. Use the hotkeys from your control surface

## Option 4: Native Plugin Development (Advanced)

For a truly integrated experience, you would need:

1. **C/C++ Development**: Write a native OBS plugin
2. **Qt Framework**: For the UI (same as OBS uses)
3. **Platform-specific builds**: Windows (.dll), macOS (.so), Linux (.so)
4. **Code signing**: Especially for macOS

This is significantly more complex than scripting but provides the best integration.

## Why These Limitations Exist:

- **Lua**: Sandboxed environment, limited to what OBS exposes
- **Python**: Can create docks but requires Python installation
- **Browser**: Can create rich UIs but lives in separate windows
- **Native**: Full access but requires compilation and distribution

## Current Best Solution:

The **Browser Source Window** approach provides the best balance of functionality and ease of use without additional dependencies. The window can be positioned to act like a dock, and modern window managers can even snap it to the side of OBS.

## Text Box Border Fix

The text box border issue in the Lua version has been fixed. The border now properly draws around the text area when "Show Text Box Outline" is enabled. The outline uses graphics primitives to draw lines forming a rectangle.
