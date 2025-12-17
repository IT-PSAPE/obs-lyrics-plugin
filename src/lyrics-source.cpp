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

	if (ls->background_texture)
		gs_texture_destroy(ls->background_texture);
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
		ls->background_file = NULL;
		if (ls->background_texture) {
			obs_enter_graphics();
			gs_texture_destroy(ls->background_texture);
			obs_leave_graphics();
			ls->background_texture = NULL;
		}
	}

	if (!ls->background_file && background_file && *background_file) {
		ls->background_file = bstrdup(background_file);
		gs_image_file_t image;
		gs_image_file_init(&image, ls->background_file);
		obs_enter_graphics();
		gs_image_file_init_texture(&image);
		ls->background_texture = image.texture;
		obs_leave_graphics();
		gs_image_file_free(&image);
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
	ls->text_width = (int)obs_data_get_int(settings, TEXT_WIDTH);
	ls->text_height = (int)obs_data_get_int(settings, TEXT_HEIGHT);

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
	ls->lyrics_folder = bstrdup(lyrics_folder);

	if (ls->lyrics_files)
		obs_data_array_release(ls->lyrics_files);
	ls->lyrics_files = obs_data_get_array(settings, LYRICS_FILES);

	load_lyrics_files(ls);
	update_text_source(ls);
}

void lyrics_source_render(void *data, gs_effect_t *effect)
{
	lyrics_source *ls = (lyrics_source *)data;

	// Render background
	if (ls->background_texture) {
		gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"), ls->background_texture);
		gs_draw_sprite(ls->background_texture, 0, gs_texture_get_width(ls->background_texture),
			       gs_texture_get_height(ls->background_texture));
	}

	// Render text
	if (ls->text_source) {
		obs_source_video_render(ls->text_source);
	}
}

uint32_t lyrics_source_get_width(void *data)
{
	lyrics_source *ls = (lyrics_source *)data;
	if (ls->background_texture)
		return gs_texture_get_width(ls->background_texture);
	return 1920; // Default width
}

uint32_t lyrics_source_get_height(void *data)
{
	lyrics_source *ls = (lyrics_source *)data;
	if (ls->background_texture)
		return gs_texture_get_height(ls->background_texture);
	return 1080; // Default height
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
