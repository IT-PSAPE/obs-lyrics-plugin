#pragma once

#include <obs-module.h>

#define TEXT_FONT_NAME "font_name"
#define TEXT_FONT_SIZE "font_size"
#define TEXT_FONT_WEIGHT "font_weight"
#define TEXT_COLOR "text_color"
#define TEXT_OUTLINE "outline"
#define TEXT_OUTLINE_SIZE "outline_size"
#define TEXT_OUTLINE_COLOR "outline_color"
#define TEXT_SHADOW "shadow"
#define TEXT_SHADOW_OFFSET_X "shadow_offset_x"
#define TEXT_SHADOW_OFFSET_Y "shadow_offset_y"
#define TEXT_SHADOW_COLOR "shadow_color"
#define TEXT_H_ALIGN "h_align"
#define TEXT_V_ALIGN "v_align"
#define TEXT_WIDTH "text_width"
#define TEXT_HEIGHT "text_height"
#define BACKGROUND_FILE "background_file"
#define LYRICS_FOLDER "lyrics_folder"
#define LYRICS_FILES "lyrics_files"
#define USE_FOLDER "use_folder"

#ifdef __cplusplus
extern "C" {
#endif

struct lyrics_source {
	obs_source_t *source;

	// Background image
	gs_texture_t *background_texture;
	char *background_file;

	// Lyrics data - using void* to hide C++ implementation
	void *songs_data;
	void *song_names_data;
	int current_song;
	int current_line;
	bool text_visible;

	// Text rendering
	obs_source_t *text_source;

	// Text properties
	uint32_t text_color;
	uint32_t outline_color;
	bool outline_enabled;
	int outline_size;
	bool shadow_enabled;
	int shadow_offset_x;
	int shadow_offset_y;
	uint32_t shadow_color;

	// Text positioning
	int text_h_align; // 0 = left, 1 = center, 2 = right
	int text_v_align; // 0 = top, 1 = center, 2 = bottom
	int text_width;
	int text_height;

	// Font settings
	char *font_name;
	int font_size;
	int font_weight;

	// File management
	char *lyrics_folder;
	obs_data_array_t *lyrics_files;
	bool use_folder;
};

// Source functions
const char *lyrics_source_get_name(void *unused);
void *lyrics_source_create(obs_data_t *settings, obs_source_t *source);
void lyrics_source_destroy(void *data);
void lyrics_source_update(void *data, obs_data_t *settings);
void lyrics_source_render(void *data, gs_effect_t *effect);
uint32_t lyrics_source_get_width(void *data);
uint32_t lyrics_source_get_height(void *data);
obs_properties_t *lyrics_source_properties(void *data);
void lyrics_source_get_defaults(obs_data_t *settings);

// Toolbar actions
void lyrics_source_next(void *data);
void lyrics_source_previous(void *data);
void lyrics_source_toggle_text(void *data);

#ifdef __cplusplus
}
#endif
