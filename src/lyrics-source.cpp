#include "lyrics-source.h"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMainWindow>
#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QStringList>
#include <graphics/image-file.h>
#include <graphics/vec4.h>
#include <vector>
#include <cmath>

// Internal data structure to hold Qt types
struct lyrics_source_data {
	std::vector<QStringList> songs;
	std::vector<QString> song_names;
};

static void load_lyrics_from_file(lyrics_source *ls, const QString &filepath)
{
	lyrics_source_data *data = static_cast<lyrics_source_data *>(ls->songs_data);

	QFile file(filepath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QTextStream in(&file);
	QStringList lines;
	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();
		if (!line.isEmpty())
			lines.append(line);
	}

	if (!lines.isEmpty()) {
		data->songs.push_back(lines);
		QFileInfo fileInfo(filepath);
		data->song_names.push_back(fileInfo.baseName());
	}
}

static void load_lyrics_files(lyrics_source *ls)
{
	lyrics_source_data *data = static_cast<lyrics_source_data *>(ls->songs_data);
	data->songs.clear();
	data->song_names.clear();
	ls->current_song = 0;
	ls->current_line = 0;

	if (ls->use_folder && ls->lyrics_folder) {
		QDir dir(ls->lyrics_folder);
		QStringList filters;
		filters << "*.txt";
		dir.setNameFilters(filters);

		QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Readable);
		for (const QFileInfo &fileInfo : files) {
			load_lyrics_from_file(ls, fileInfo.absoluteFilePath());
		}
	} else if (ls->lyrics_files) {
		size_t count = obs_data_array_count(ls->lyrics_files);
		for (size_t i = 0; i < count; i++) {
			obs_data_t *item = obs_data_array_item(ls->lyrics_files, i);
			const char *filepath = obs_data_get_string(item, "value");
			if (filepath && *filepath)
				load_lyrics_from_file(ls, QString::fromUtf8(filepath));
			obs_data_release(item);
		}
	}
}

static void update_text_source(lyrics_source *ls)
{
	if (!ls->text_source)
		return;

	lyrics_source_data *data = static_cast<lyrics_source_data *>(ls->songs_data);
	obs_data_t *settings = obs_data_create();

	// Set text content
	QString text;
	if (ls->text_visible && ls->current_song >= 0 && ls->current_song < (int)data->songs.size() &&
	    ls->current_line >= 0 && ls->current_line < data->songs[ls->current_song].size()) {
		text = data->songs[ls->current_song][ls->current_line];
	}

	obs_data_set_string(settings, "text", text.toUtf8().constData());

	// Font settings
	obs_data_t *font = obs_data_create();
	obs_data_set_string(font, "face", ls->font_name ? ls->font_name : "Arial");
	obs_data_set_int(font, "size", ls->font_size);
	obs_data_set_int(font, "style", ls->font_weight);
	obs_data_set_obj(settings, "font", font);
	obs_data_release(font);

	// Colors
	obs_data_set_int(settings, "color", ls->text_color);
	obs_data_set_bool(settings, "outline", ls->outline_enabled);
	obs_data_set_int(settings, "outline_size", ls->outline_size);
	obs_data_set_int(settings, "outline_color", ls->outline_color);

	// Shadow
	obs_data_set_bool(settings, "drop_shadow", ls->shadow_enabled);
	const int shadow_distance =
		(int)std::lround(std::sqrt((double)ls->shadow_offset_x * (double)ls->shadow_offset_x +
					   (double)ls->shadow_offset_y * (double)ls->shadow_offset_y));
	obs_data_set_int(settings, "shadow_distance", shadow_distance);
	obs_data_set_int(settings, "shadow_color", ls->shadow_color);

	// Alignment
	const char *align_str = "center";
	if (ls->text_h_align == 0 && ls->text_v_align == 0)
		align_str = "top_left";
	else if (ls->text_h_align == 1 && ls->text_v_align == 0)
		align_str = "top_center";
	else if (ls->text_h_align == 2 && ls->text_v_align == 0)
		align_str = "top_right";
	else if (ls->text_h_align == 0 && ls->text_v_align == 1)
		align_str = "center_left";
	else if (ls->text_h_align == 1 && ls->text_v_align == 1)
		align_str = "center";
	else if (ls->text_h_align == 2 && ls->text_v_align == 1)
		align_str = "center_right";
	else if (ls->text_h_align == 0 && ls->text_v_align == 2)
		align_str = "bottom_left";
	else if (ls->text_h_align == 1 && ls->text_v_align == 2)
		align_str = "bottom_center";
	else if (ls->text_h_align == 2 && ls->text_v_align == 2)
		align_str = "bottom_right";
	obs_data_set_string(settings, "align", align_str);

	// Word wrap
	obs_data_set_bool(settings, "wrap", true);
	obs_data_set_int(settings, "extents_width", ls->text_width);
	obs_data_set_int(settings, "extents_height", ls->text_height);
	obs_data_set_bool(settings, "extents", true);

	obs_source_update(ls->text_source, settings);
	obs_data_release(settings);
}

static void unload_background_image(lyrics_source *ls)
{
	if (!ls->background_loaded)
		return;

	obs_enter_graphics();
	gs_image_file4_free(&ls->background_image);
	obs_leave_graphics();

	ls->background_loaded = false;
}

static void load_background_image(lyrics_source *ls)
{
	unload_background_image(ls);

	if (!ls->background_file || !*ls->background_file)
		return;

	gs_image_file4_init(&ls->background_image, ls->background_file, GS_IMAGE_ALPHA_PREMULTIPLY);

	obs_enter_graphics();
	gs_image_file4_init_texture(&ls->background_image);
	obs_leave_graphics();

	ls->background_loaded = true;
}

static void draw_bounds_outline(int x, int y, int w, int h, int thickness, uint32_t rgba)
{
	if (w <= 0 || h <= 0 || thickness <= 0)
		return;

	gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_eparam_t *color_param = gs_effect_get_param_by_name(solid, "color");
	gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");

	struct vec4 color;
	vec4_from_rgba(&color, rgba);
	gs_effect_set_vec4(color_param, &color);

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);

	// top
	gs_matrix_push();
	gs_matrix_translate3f((float)x, (float)y, 0.0f);
	gs_draw_sprite(nullptr, 0, w, thickness);
	gs_matrix_pop();

	// bottom
	gs_matrix_push();
	gs_matrix_translate3f((float)x, (float)(y + h - thickness), 0.0f);
	gs_draw_sprite(nullptr, 0, w, thickness);
	gs_matrix_pop();

	// left
	gs_matrix_push();
	gs_matrix_translate3f((float)x, (float)y, 0.0f);
	gs_draw_sprite(nullptr, 0, thickness, h);
	gs_matrix_pop();

	// right
	gs_matrix_push();
	gs_matrix_translate3f((float)(x + w - thickness), (float)y, 0.0f);
	gs_draw_sprite(nullptr, 0, thickness, h);
	gs_matrix_pop();

	gs_technique_end_pass(tech);
	gs_technique_end(tech);
}

const char *lyrics_source_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("LyricsSource");
}

void *lyrics_source_create(obs_data_t *settings, obs_source_t *source)
{
	lyrics_source *ls = (lyrics_source *)bzalloc(sizeof(lyrics_source));
	ls->source = source;

	// Create internal data structure
	ls->songs_data = new lyrics_source_data();

	// Initialize defaults
	ls->text_visible = true;
	ls->current_song = 0;
	ls->current_line = 0;

	// Create text source
	obs_data_t *text_settings = obs_data_create();
	ls->text_source = obs_source_create_private("text_ft2_source", "lyrics_text", text_settings);
	obs_data_release(text_settings);

	// Register hotkeys
	obs_hotkey_register_source(
		source, "lyrics.next", obs_module_text("NextLyric"),
		[](void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed) {
			UNUSED_PARAMETER(id);
			UNUSED_PARAMETER(hotkey);
			if (pressed)
				lyrics_source_next(data);
		},
		ls);

	obs_hotkey_register_source(
		source, "lyrics.prev", obs_module_text("PreviousLyric"),
		[](void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed) {
			UNUSED_PARAMETER(id);
			UNUSED_PARAMETER(hotkey);
			if (pressed)
				lyrics_source_previous(data);
		},
		ls);

	obs_hotkey_register_source(
		source, "lyrics.toggle", obs_module_text("ShowHideLyrics"),
		[](void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed) {
			UNUSED_PARAMETER(id);
			UNUSED_PARAMETER(hotkey);
			if (pressed)
				lyrics_source_toggle_text(data);
		},
		ls);

	lyrics_source_update(ls, settings);

	return ls;
}

void lyrics_source_destroy(void *data)
{
	lyrics_source *ls = (lyrics_source *)data;

	unload_background_image(ls);
	if (ls->text_source)
		obs_source_release(ls->text_source);

	// Delete internal data structure
	if (ls->songs_data)
		delete static_cast<lyrics_source_data *>(ls->songs_data);

	bfree(ls->background_file);
	bfree(ls->font_name);
	bfree(ls->lyrics_folder);
	if (ls->lyrics_files)
		obs_data_array_release(ls->lyrics_files);

	bfree(ls);
}

void lyrics_source_update(void *data, obs_data_t *settings)
{
	lyrics_source *ls = (lyrics_source *)data;

	// Update background image
	const char *background_file = obs_data_get_string(settings, BACKGROUND_FILE);
	if (ls->background_file && strcmp(ls->background_file, background_file) != 0) {
		bfree(ls->background_file);
		ls->background_file = nullptr;
		unload_background_image(ls);
	}

	if (!ls->background_file && background_file && *background_file) {
		ls->background_file = bstrdup(background_file);
		load_background_image(ls);
	}

	// Update text properties
	ls->text_color = (uint32_t)obs_data_get_int(settings, TEXT_COLOR);
	ls->outline_enabled = obs_data_get_bool(settings, TEXT_OUTLINE);
	ls->outline_size = (int)obs_data_get_int(settings, TEXT_OUTLINE_SIZE);
	ls->outline_color = (uint32_t)obs_data_get_int(settings, TEXT_OUTLINE_COLOR);
	ls->shadow_enabled = obs_data_get_bool(settings, TEXT_SHADOW);
	ls->shadow_offset_x = (int)obs_data_get_int(settings, TEXT_SHADOW_OFFSET_X);
	ls->shadow_offset_y = (int)obs_data_get_int(settings, TEXT_SHADOW_OFFSET_Y);
	ls->shadow_color = (uint32_t)obs_data_get_int(settings, TEXT_SHADOW_COLOR);

	// Update alignment
	ls->text_h_align = (int)obs_data_get_int(settings, TEXT_H_ALIGN);
	ls->text_v_align = (int)obs_data_get_int(settings, TEXT_V_ALIGN);
	ls->text_x = (int)obs_data_get_int(settings, TEXT_X);
	ls->text_y = (int)obs_data_get_int(settings, TEXT_Y);
	ls->text_width = (int)obs_data_get_int(settings, TEXT_WIDTH);
	ls->text_height = (int)obs_data_get_int(settings, TEXT_HEIGHT);
	ls->show_bounds = obs_data_get_bool(settings, TEXT_SHOW_BOUNDS);
	ls->bounds_color = (uint32_t)obs_data_get_int(settings, TEXT_BOUNDS_COLOR);
	ls->bounds_thickness = (int)obs_data_get_int(settings, TEXT_BOUNDS_THICKNESS);

	// Update font
	const char *font_name = obs_data_get_string(settings, TEXT_FONT_NAME);
	if (ls->font_name)
		bfree(ls->font_name);
	ls->font_name = bstrdup(font_name && *font_name ? font_name : "Arial");
	ls->font_size = (int)obs_data_get_int(settings, TEXT_FONT_SIZE);
	ls->font_weight = (int)obs_data_get_int(settings, TEXT_FONT_WEIGHT);

	// Update lyrics files
	ls->use_folder = obs_data_get_bool(settings, USE_FOLDER);
	const char *lyrics_folder = obs_data_get_string(settings, LYRICS_FOLDER);
	if (ls->lyrics_folder)
		bfree(ls->lyrics_folder);
	ls->lyrics_folder = (lyrics_folder && *lyrics_folder) ? bstrdup(lyrics_folder) : nullptr;

	if (ls->lyrics_files)
		obs_data_array_release(ls->lyrics_files);
	ls->lyrics_files = obs_data_get_array(settings, LYRICS_FILES);

	load_lyrics_files(ls);
	update_text_source(ls);
}

void lyrics_source_render(void *data, gs_effect_t *effect)
{
	lyrics_source *ls = (lyrics_source *)data;

	gs_effect_t *draw_effect = effect;
	if (!draw_effect)
		draw_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);

	// Render background
	if (ls->background_loaded) {
		struct gs_image_file *const image = &ls->background_image.image3.image2.image;
		gs_texture_t *const texture = image->texture;
		if (texture) {
			const bool previous = gs_framebuffer_srgb_enabled();
			gs_enable_framebuffer_srgb(true);

			gs_blend_state_push();
			gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

			gs_eparam_t *const param = gs_effect_get_param_by_name(draw_effect, "image");
			gs_effect_set_texture_srgb(param, texture);

			for (gs_effect_loop(draw_effect, "Draw")) {
				gs_draw_sprite(texture, 0, image->cx, image->cy);
			}

			gs_blend_state_pop();
			gs_enable_framebuffer_srgb(previous);
		}
	}

	// Bounds preview overlay
	if (ls->show_bounds) {
		draw_bounds_outline(ls->text_x, ls->text_y, ls->text_width, ls->text_height, ls->bounds_thickness,
				   ls->bounds_color);
	}

	// Render text translated into position
	if (ls->text_source) {
		gs_matrix_push();
		gs_matrix_translate3f((float)ls->text_x, (float)ls->text_y, 0.0f);
		obs_source_video_render(ls->text_source);
		gs_matrix_pop();
	}
}

uint32_t lyrics_source_get_width(void *data)
{
	lyrics_source *ls = (lyrics_source *)data;
	if (ls->background_loaded && ls->background_image.image3.image2.image.cx > 0)
		return ls->background_image.image3.image2.image.cx;
	return 1920; // Default width
}

uint32_t lyrics_source_get_height(void *data)
{
	lyrics_source *ls = (lyrics_source *)data;
	if (ls->background_loaded && ls->background_image.image3.image2.image.cy > 0)
		return ls->background_image.image3.image2.image.cy;
	return 1080; // Default height
}

void lyrics_source_media_play_pause(void *data, bool pause)
{
	lyrics_source *ls = (lyrics_source *)data;
	ls->text_visible = !pause;
	update_text_source(ls);
}

void lyrics_source_media_restart(void *data)
{
	lyrics_source *ls = (lyrics_source *)data;
	ls->current_song = 0;
	ls->current_line = 0;
	ls->text_visible = true;
	update_text_source(ls);
}

void lyrics_source_media_stop(void *data)
{
	lyrics_source *ls = (lyrics_source *)data;
	ls->current_song = 0;
	ls->current_line = 0;
	ls->text_visible = false;
	update_text_source(ls);
}

void lyrics_source_media_next(void *data)
{
	lyrics_source_next(data);
}

void lyrics_source_media_previous(void *data)
{
	lyrics_source_previous(data);
}

enum obs_media_state lyrics_source_media_get_state(void *data)
{
	lyrics_source *ls = (lyrics_source *)data;
	return ls->text_visible ? OBS_MEDIA_STATE_PLAYING : OBS_MEDIA_STATE_PAUSED;
}

void lyrics_source_next(void *data)
{
	lyrics_source *ls = (lyrics_source *)data;
	lyrics_source_data *ldata = static_cast<lyrics_source_data *>(ls->songs_data);
	if (ldata->songs.empty())
		return;

	const int song_line_count = static_cast<int>(ldata->songs[ls->current_song].size());

	ls->current_line++;
	if (ls->current_line >= song_line_count) {
		ls->current_line = 0;
		ls->current_song++;
		if (ls->current_song >= static_cast<int>(ldata->songs.size()))
			ls->current_song = 0;
	}

	update_text_source(ls);
}

void lyrics_source_previous(void *data)
{
	lyrics_source *ls = (lyrics_source *)data;
	lyrics_source_data *ldata = static_cast<lyrics_source_data *>(ls->songs_data);
	if (ldata->songs.empty())
		return;

	ls->current_line--;
	if (ls->current_line < 0) {
		ls->current_song--;
		if (ls->current_song < 0)
			ls->current_song = static_cast<int>(ldata->songs.size()) - 1;
		const int previous_song_lines = static_cast<int>(ldata->songs[ls->current_song].size());
		ls->current_line = previous_song_lines - 1;
	}

	update_text_source(ls);
}

void lyrics_source_toggle_text(void *data)
{
	lyrics_source *ls = (lyrics_source *)data;
	ls->text_visible = !ls->text_visible;
	update_text_source(ls);
}
