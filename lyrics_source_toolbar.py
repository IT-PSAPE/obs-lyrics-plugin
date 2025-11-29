import obspython as obs
import json

# JavaScript code to inject toolbar buttons
toolbar_js = """
(function() {
    // Wait for OBS Studio to be ready
    if (!window.obsstudio || !window.obsstudio.pluginVersion) {
        setTimeout(arguments.callee, 100);
        return;
    }
    
    // Function to add lyrics controls to source toolbar
    function addLyricsControls() {
        // Find all source toolbars
        const toolbars = document.querySelectorAll('.source-toolbar');
        
        toolbars.forEach(toolbar => {
            // Check if we already added controls
            if (toolbar.querySelector('.lyrics-controls')) {
                return;
            }
            
            // Create lyrics control container
            const controlsDiv = document.createElement('div');
            controlsDiv.className = 'lyrics-controls';
            controlsDiv.style.cssText = 'display: inline-block; margin-left: 10px;';
            
            // Create control button
            const controlBtn = document.createElement('button');
            controlBtn.className = 'button button--default';
            controlBtn.innerHTML = '🎵 Lyrics Control';
            controlBtn.title = 'Open Lyrics Control Panel';
            controlBtn.onclick = function() {
                // Send message to Python to open control panel
                window.obsstudio.sendMessage('open-lyrics-control', {
                    source: toolbar.getAttribute('data-source-name')
                });
            };
            
            // Create previous button
            const prevBtn = document.createElement('button');
            prevBtn.className = 'button button--default';
            prevBtn.innerHTML = '◀';
            prevBtn.title = 'Previous Line';
            prevBtn.onclick = function() {
                window.obsstudio.sendMessage('lyrics-previous', {});
            };
            
            // Create next button
            const nextBtn = document.createElement('button');
            nextBtn.className = 'button button--default';
            nextBtn.innerHTML = '▶';
            nextBtn.title = 'Next Line';
            nextBtn.onclick = function() {
                window.obsstudio.sendMessage('lyrics-next', {});
            };
            
            // Create hide/show button
            const hideBtn = document.createElement('button');
            hideBtn.className = 'button button--default';
            hideBtn.innerHTML = '👁';
            hideBtn.title = 'Toggle Lyrics Visibility';
            hideBtn.onclick = function() {
                window.obsstudio.sendMessage('lyrics-toggle', {});
            };
            
            // Add buttons to container
            controlsDiv.appendChild(prevBtn);
            controlsDiv.appendChild(nextBtn);
            controlsDiv.appendChild(hideBtn);
            controlsDiv.appendChild(controlBtn);
            
            // Add to toolbar
            toolbar.appendChild(controlsDiv);
        });
    }
    
    // Observe for new toolbars being added
    const observer = new MutationObserver(function(mutations) {
        mutations.forEach(function(mutation) {
            if (mutation.addedNodes.length) {
                addLyricsControls();
            }
        });
    });
    
    // Start observing
    observer.observe(document.body, {
        childList: true,
        subtree: true
    });
    
    // Initial run
    addLyricsControls();
})();
"""

# Alternative approach using OBS frontend API
def add_source_toolbar_actions():
    """Add actions to source context menu"""
    
    # Previous line action
    prev_action = obs.obs_frontend_add_tools_menu_item("Lyrics: Previous Line")
    if prev_action:
        obs.obs_frontend_add_event_callback(prev_action, lambda: previous_line_callback())
    
    # Next line action  
    next_action = obs.obs_frontend_add_tools_menu_item("Lyrics: Next Line")
    if next_action:
        obs.obs_frontend_add_event_callback(next_action, lambda: next_line_callback())
    
    # Toggle visibility action
    toggle_action = obs.obs_frontend_add_tools_menu_item("Lyrics: Toggle Visibility")
    if toggle_action:
        obs.obs_frontend_add_event_callback(toggle_action, lambda: toggle_visibility_callback())
    
    # Open control panel action
    control_action = obs.obs_frontend_add_tools_menu_item("Lyrics: Open Control Panel")
    if control_action:
        obs.obs_frontend_add_event_callback(control_action, lambda: open_control_panel())

def previous_line_callback():
    """Callback for previous line action"""
    from obs_lyrics_plugin import previous_line
    previous_line(True)

def next_line_callback():
    """Callback for next line action"""
    from obs_lyrics_plugin import next_line
    next_line(True)

def toggle_visibility_callback():
    """Callback for toggle visibility action"""
    from obs_lyrics_plugin import toggle_visibility
    toggle_visibility(True)

def open_control_panel():
    """Open the lyrics control dock"""
    # This would open the control dock if it's closed
    pass

def script_load(settings):
    """Called when script is loaded"""
    add_source_toolbar_actions()

def script_description():
    return "Lyrics Source Toolbar Integration - Adds lyrics controls to source toolbar"

# Context menu integration
def add_source_context_menu():
    """Add items to source context menu"""
    
    def source_context_menu_callback(source):
        """Called when right-clicking a source"""
        if not source:
            return
        
        source_id = obs.obs_source_get_id(source)
        
        # Only add menu items for text sources
        if source_id in ["text_gdiplus", "text_ft2_source"]:
            menu = obs.obs_frontend_get_current_preview_scene_menu()
            if menu:
                # Add separator
                menu.addSeparator()
                
                # Add lyrics actions
                prev_action = menu.addAction("Lyrics: Previous Line")
                prev_action.triggered.connect(lambda: previous_line_callback())
                
                next_action = menu.addAction("Lyrics: Next Line")
                next_action.triggered.connect(lambda: next_line_callback())
                
                toggle_action = menu.addAction("Lyrics: Toggle Visibility")
                toggle_action.triggered.connect(lambda: toggle_visibility_callback())
                
                control_action = menu.addAction("Lyrics: Control Panel")
                control_action.triggered.connect(lambda: open_control_panel())
    
    # Register context menu callback
    obs.obs_frontend_add_save_callback(lambda: None)  # Placeholder for proper implementation
