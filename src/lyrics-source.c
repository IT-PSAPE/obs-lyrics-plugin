#include "lyrics-source.h"
#include <util/platform.h>
#include <util/dstr.h>
#include <graphics/graphics.h>
#include <graphics/image-file.h>
#include <obs-module.h>
#include <sys/types.h>
#include <dirent.h>

/* ------------------------------------------------------------------------- */
/* Data Structure                                                            */

struct lyrics_source {
	obs_source_t *source;

	/* Properties */
	char *image_path;
	char *lyrics_folder;
	char *font_family;
	char *font_style;
	uint32_t font_size;
	uint32_t text_color;
	bool outline_enabled;
	uint32_t outline_color;
	uint32_t outline_size;
	
	const char *align_h;
	const char *align_v;
	float box_width;
	float box_height;

	/* Runtime Data */
	gs_image_file_t background_image;
	
	/* Text Rendering */
	obs_source_t *text_source; /* We will use a child text source for easier rendering */
	
	/* Lyrics Data */
	struct dstr_array files;
	struct dstr_array current_lyrics;
	int current_file_index;
	int current_line_index;
	bool visible;
};

/* ------------------------------------------------------------------------- */
/* Helper Functions                                                          */

static void update_text_source(struct lyrics_source *context)
{
	if (!context->text_source) return;

	obs_data_t *settings = obs_data_create();

	/* Set Font */
	obs_data_t *font_obj = obs_data_create();
	obs_data_set_string(font_obj, "face", context->font_family);
	obs_data_set_string(font_obj, "style", context->font_style);
	obs_data_set_int(font_obj, "size", context->font_size);
	obs_data_set_obj(settings, "font", font_obj);
	obs_data_release(font_obj);

	/* Set Color */
	obs_data_set_int(settings, "color", context->text_color);
	
	/* Set Outline */
	obs_data_set_bool(settings, "outline", context->outline_enabled);
	obs_data_set_int(settings, "outline_color", context->outline_color);
	obs_data_set_int(settings, "outline_size", context->outline_size);

	/* Set Text Content */
	const char *text = "";
	if (context->visible && context->current_lyrics.num > 0 && 
	    context->current_line_index >= 0 && 
	    (size_t)context->current_line_index < context->current_lyrics.num) {
		text = context->current_lyrics.array[context->current_line_index].array;
	}
	obs_data_set_string(settings, "text", text);

	/* Set Alignment/Bounds (if using text source bounds) */
	/* For now, we will handle positioning in render, but text source needs to know alignment for multiline or just origin */
	obs_data_set_string(settings, "align", context->align_h);
	obs_data_set_string(settings, "valign", context->align_v);
	
	/* Update the child source */
	obs_source_update(context->text_source, settings);
	obs_data_release(settings);
}

static void load_lyrics_file(struct lyrics_source *context)
{
	dstr_array_free(context->current_lyrics);
	context->current_line_index = 0;

	if (context->files.num == 0 || context->current_file_index < 0 || 
	    (size_t)context->current_file_index >= context->files.num) {
		return;
	}

	struct dstr path = {0};
	dstr_copy(&path, context->lyrics_folder);
	if (dstr_end(&path) != '/' && dstr_end(&path) != '\\')
		dstr_cat_ch(&path, '/');
	dstr_cat(&path, context->files.array[context->current_file_index].array);

	char *content = os_quick_read_utf8_file(path.array);
	if (content) {
		dstr_split(&context->current_lyrics, content, "\n");
		bfree(content);
		
		/* Clean up newlines */
		for (size_t i = 0; i < context->current_lyrics.num; i++) {
			dstr_depad(&context->current_lyrics.array[i]);
		}
	}
	dstr_free(&path);
	
	update_text_source(context);
}

static void refresh_files(struct lyrics_source *context)
{
	dstr_array_free(context->files);
	
	if (!context->lyrics_folder || !*context->lyrics_folder)
		return;

	DIR *dir = opendir(context->lyrics_folder);
	if (!dir) return;

	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		if (ent->d_name[0] == '.') continue;
		const char *ext = strrchr(ent->d_name, '.');
		if (ext && strcasecmp(ext, ".txt") == 0) {
			struct dstr name = {0};
			dstr_copy(&name, ent->d_name);
			dstr_array_push_back(&context->files, &name);
		}
	}
	closedir(dir);
	
	/* Sort files alphabetically? For now, OS order (usually undefined/creation) */
	/* TODO: Sort */
}

/* ------------------------------------------------------------------------- */
/* Source Lifecycle                                                          */

static const char *lyrics_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "Lyrics Source";
}

static void *lyrics_create(obs_data_t *settings, obs_source_t *source)
{
	struct lyrics_source *context = bzalloc(sizeof(struct lyrics_source));
	context->source = source;
	context->visible = true;

	/* Create child text source (Text GDI+ or Freetype 2) */
	/* We try to use the standard text source available on the platform */
#ifdef _WIN32
	const char *text_id = "text_gdiplus";
#else
	const char *text_id = "text_ft2_source";
#endif
	context->text_source = obs_source_create_private(text_id, "lyrics_text", NULL);

	obs_source_update(source, settings);
	return context;
}

static void lyrics_destroy(void *data)
{
	struct lyrics_source *context = data;
	
	if (context->text_source) {
		obs_source_release(context->text_source);
	}
	
	gs_image_file_free(&context->background_image);
	bfree(context->image_path);
	bfree(context->lyrics_folder);
	bfree(context->font_family);
	bfree(context->font_style);
	
	dstr_array_free(context->files);
	dstr_array_free(context->current_lyrics);
	
	bfree(context);
}

static void lyrics_update(void *data, obs_data_t *settings)
{
	struct lyrics_source *context = data;

	/* Image */
	const char *path = obs_data_get_string(settings, "image_path");
	if (path && (!context->image_path || strcmp(path, context->image_path) != 0)) {
		bfree(context->image_path);
		context->image_path = bstrdup(path);
		gs_image_file_free(&context->background_image);
		gs_image_file_init(&context->background_image, path);
	}

	/* Folder */
	const char *folder = obs_data_get_string(settings, "lyrics_folder");
	bool folder_changed = false;
	if (folder && (!context->lyrics_folder || strcmp(folder, context->lyrics_folder) != 0)) {
		bfree(context->lyrics_folder);
		context->lyrics_folder = bstrdup(folder);
		folder_changed = true;
	}

	/* Positioning */
	context->align_h = obs_data_get_string(settings, "align_h");
	context->align_v = obs_data_get_string(settings, "align_v");
	context->box_width = (float)obs_data_get_double(settings, "box_width");
	context->box_height = (float)obs_data_get_double(settings, "box_height");

	/* Font */
	obs_data_t *font_obj = obs_data_get_obj(settings, "font");
	if (font_obj) {
		bfree(context->font_family);
		bfree(context->font_style);
		context->font_family = bstrdup(obs_data_get_string(font_obj, "face"));
		context->font_style = bstrdup(obs_data_get_string(font_obj, "style"));
		context->font_size = (uint32_t)obs_data_get_int(font_obj, "size");
		obs_data_release(font_obj);
	}
	
	context->text_color = (uint32_t)obs_data_get_int(settings, "color");
	context->outline_enabled = obs_data_get_bool(settings, "outline");
	context->outline_color = (uint32_t)obs_data_get_int(settings, "outline_color");
	context->outline_size = (uint32_t)obs_data_get_int(settings, "outline_size");

	/* Control Properties */
	const char *song_file = obs_data_get_string(settings, "current_song_file");
	if (song_file && *song_file) {
		/* Find index */
		for (size_t i = 0; i < context->files.num; i++) {
			if (strcmp(context->files.array[i].array, song_file) == 0) {
				if (context->current_file_index != (int)i) {
					context->current_file_index = (int)i;
					load_lyrics_file(context);
				}
				break;
			}
		}
	}

	int line_idx = (int)obs_data_get_int(settings, "current_line_index");
	if (line_idx != context->current_line_index) {
		context->current_line_index = line_idx;
		/* Bounds check */
		if (context->current_line_index < 0) context->current_line_index = 0;
		if (context->current_lyrics.num > 0 && (size_t)context->current_line_index >= context->current_lyrics.num)
			context->current_line_index = (int)context->current_lyrics.num - 1;
	}

	bool hidden = obs_data_get_bool(settings, "lyrics_hidden");
	context->visible = !hidden;

	if (folder_changed) {
		refresh_files(context);
		/* If song_file was set, try to find it in new folder, else default to 0 */
		context->current_file_index = 0;
		if (song_file && *song_file) {
			for (size_t i = 0; i < context->files.num; i++) {
				if (strcmp(context->files.array[i].array, song_file) == 0) {
					context->current_file_index = (int)i;
					break;
				}
			}
		}
		load_lyrics_file(context);
	} else {
		update_text_source(context);
	}
}

static void lyrics_video_render(void *data, gs_effect_t *effect)
{
	struct lyrics_source *context = data;
	UNUSED_PARAMETER(effect);

	/* Draw Background */
	if (context->background_image.texture) {
		gs_draw_sprite(&context->background_image, 0, 0, 0);
	}

	/* Draw Text */
	if (context->text_source) {
		/* Calculate position based on alignment and box size */
		/* For simplicity, we assume the source size matches the image size or a fixed size */
		/* But we need to know the size of the text source to center it within the box */
		
		uint32_t text_width = obs_source_get_width(context->text_source);
		uint32_t text_height = obs_source_get_height(context->text_source);
		
		float x = 0.0f;
		float y = 0.0f;

		/* Horizontal Alignment in Box */
		if (strcmp(context->align_h, "center") == 0) {
			x = (context->box_width - text_width) / 2.0f;
		} else if (strcmp(context->align_h, "right") == 0) {
			x = context->box_width - text_width;
		}
		
		/* Vertical Alignment in Box */
		if (strcmp(context->align_v, "center") == 0) {
			y = (context->box_height - text_height) / 2.0f;
		} else if (strcmp(context->align_v, "bottom") == 0) {
			y = context->box_height - text_height;
		}

		/* We also need to consider where the box itself is? 
		   The user said "set the position of the text box". 
		   Usually this means the box is relative to the source origin (0,0).
		   The source itself can be moved in the scene.
		   So we just render at x,y relative to 0,0. 
		   Wait, the user said "position it left, center, right". 
		   Maybe they mean the text within the box? Or the box within the screen?
		   "we can position it using the... horizontally... left, center, right"
		   "change the width of the text box and height"
		   
		   Let's assume the "Box" starts at (0,0) of the source, and has size (box_width, box_height).
		   The text is aligned WITHIN that box.
		   The source size should probably be the size of the background image, or the box size if no image?
		   Let's make the source size the max of image and box.
		*/

		gs_matrix_push();
		gs_matrix_translate3f(x, y, 0.0f);
		obs_source_video_render(context->text_source);
		gs_matrix_pop();
	}
}

static uint32_t lyrics_get_width(void *data)
{
	struct lyrics_source *context = data;
	uint32_t w = context->background_image.cx;
	if (context->box_width > w) w = (uint32_t)context->box_width;
	return w;
}

static uint32_t lyrics_get_height(void *data)
{
	struct lyrics_source *data_ = data;
	uint32_t h = data_->background_image.cy;
	if (data_->box_height > h) h = (uint32_t)data_->box_height;
	return h;
}

/* ------------------------------------------------------------------------- */
/* Properties                                                                */

static obs_properties_t *lyrics_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_path(props, "image_path", "Background Image", OBS_PATH_FILE, "Image Files (*.bmp *.jpg *.jpeg *.tga *.gif *.png)", NULL);
	obs_properties_add_path(props, "lyrics_folder", "Lyrics Folder", OBS_PATH_DIRECTORY, NULL, NULL);

	obs_properties_t *list = obs_properties_add_list(props, "align_h", "Horizontal Align", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(list, "Left", "left");
	obs_property_list_add_string(list, "Center", "center");
	obs_property_list_add_string(list, "Right", "right");

	list = obs_properties_add_list(props, "align_v", "Vertical Align", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(list, "Top", "top");
	obs_property_list_add_string(list, "Center", "center");
	obs_property_list_add_string(list, "Bottom", "bottom");

	obs_properties_add_float(props, "box_width", "Box Width", 0, 10000, 1);
	obs_properties_add_float(props, "box_height", "Box Height", 0, 10000, 1);

	obs_properties_add_font(props, "font", "Font");
	obs_properties_add_color(props, "color", "Text Color");
	
	obs_properties_add_bool(props, "outline", "Enable Outline");
	obs_properties_add_color(props, "outline_color", "Outline Color");
	obs_properties_add_int(props, "outline_size", "Outline Size", 0, 20, 1);

	return props;
}

static void lyrics_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "align_h", "center");
	obs_data_set_default_string(settings, "align_v", "bottom");
	obs_data_set_default_double(settings, "box_width", 1920.0);
	obs_data_set_default_double(settings, "box_height", 200.0);
	obs_data_set_default_int(settings, "color", 0xFFFFFFFF);
	obs_data_set_default_bool(settings, "outline", true);
	obs_data_set_default_int(settings, "outline_color", 0xFF000000);
	obs_data_set_default_int(settings, "outline_size", 2);
	
	obs_data_t *font_obj = obs_data_create();
	obs_data_set_string(font_obj, "face", "Arial");
	obs_data_set_int(font_obj, "size", 48);
	obs_data_set_default_obj(settings, "font", font_obj);
	obs_data_release(font_obj);
}

/* ------------------------------------------------------------------------- */
/* Definition                                                                */

struct obs_source_info lyrics_source_info = {
	.id = "lyrics_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = lyrics_get_name,
	.create = lyrics_create,
	.destroy = lyrics_destroy,
	.update = lyrics_update,
	.video_render = lyrics_video_render,
	.get_width = lyrics_get_width,
	.get_height = lyrics_get_height,
	.get_properties = lyrics_properties,
	.get_defaults = lyrics_defaults,
};
