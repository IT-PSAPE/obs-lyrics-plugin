obs = obslua

-- Test script to debug text rendering
function script_description()
    return [[Text Rendering Test

This script helps debug why text isn't showing in the Lyrics plugin.
Add this script and check the Script Log for debug output.]]
end

function script_load(settings)
    print("=== Text Rendering Debug Test ===")
    
    -- Test creating a text source
    local test_settings = obs.obs_data_create()
    obs.obs_data_set_string(test_settings, "text", "TEST TEXT - Can you see this?")
    obs.obs_data_set_int(test_settings, "color1", 0xFFFFFFFF)
    obs.obs_data_set_int(test_settings, "color2", 0xFFFFFFFF)
    
    local font_obj = obs.obs_data_create()
    obs.obs_data_set_string(font_obj, "face", "Arial")
    obs.obs_data_set_int(font_obj, "size", 72)
    obs.obs_data_set_string(font_obj, "style", "Bold")
    obs.obs_data_set_obj(test_settings, "font", font_obj)
    obs.obs_data_release(font_obj)
    
    print("Creating test text source...")
    local test_source = obs.obs_source_create("text_ft2_source", "test_text_debug", test_settings, nil)
    
    if test_source then
        print("✓ Text source created successfully")
        
        -- Check source dimensions
        local width = obs.obs_source_get_width(test_source)
        local height = obs.obs_source_get_height(test_source)
        print(string.format("Text source dimensions: %dx%d", width, height))
        
        -- Add to current scene for testing
        local scene_source = obs.obs_frontend_get_current_scene()
        if scene_source then
            local scene = obs.obs_scene_from_source(scene_source)
            if scene then
                print("Adding test text to current scene...")
                obs.obs_scene_add(scene, test_source)
                print("✓ Test text added to scene")
            end
            obs.obs_source_release(scene_source)
        end
        
        obs.obs_source_release(test_source)
    else
        print("✗ Failed to create text source!")
    end
    
    obs.obs_data_release(test_settings)
    
    print("=== End Debug Test ===")
end
