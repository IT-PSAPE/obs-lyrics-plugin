#include "lyrics-source.h"
#include <obs-module.h>

static bool use_folder_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	bool use_folder = obs_data_get_bool(settings, USE_FOLDER);
	obs_property_t *folder_path = obs_properties_get(props, LYRICS_FOLDER);
	obs_property_t *files_list = obs_properties_get(props, LYRICS_FILES);
	obs_property_set_visible(folder_path, use_folder);
	obs_property_set_visible(files_list, !use_folder);
	return true;
}

obs_properties_t *lyrics_source_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	// Background Image
	obs_properties_add_path(props, BACKGROUND_FILE, obs_module_text("BackgroundImage"), OBS_PATH_FILE,
				"Image Files (*.png *.jpg *.jpeg *.gif *.bmp);;All Files (*)", NULL);

	// Lyrics Source Selection
	obs_property_t *use_folder = obs_properties_add_bool(props, USE_FOLDER, obs_module_text("UseFolder"));

	obs_properties_add_path(props, LYRICS_FOLDER, obs_module_text("LyricsFolder"), OBS_PATH_DIRECTORY, NULL, NULL);

	obs_properties_add_editable_list(props, LYRICS_FILES, obs_module_text("LyricsFiles"),
					 OBS_EDITABLE_LIST_TYPE_FILES, "Text Files (*.txt);;All Files (*)", NULL);

	// Layout groups
	obs_properties_t *align_group = obs_properties_create();
	obs_property_t *h_align = obs_properties_add_list(align_group, TEXT_H_ALIGN,
					 obs_module_text("HorizontalAlignment"), OBS_COMBO_TYPE_RADIO,
					 OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(h_align, obs_module_text("Left"), 0);
	obs_property_list_add_int(h_align, obs_module_text("Center"), 1);
	obs_property_list_add_int(h_align, obs_module_text("Right"), 2);

	obs_property_t *v_align = obs_properties_add_list(align_group, TEXT_V_ALIGN,
					 obs_module_text("VerticalAlignment"), OBS_COMBO_TYPE_RADIO,
					 OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(v_align, obs_module_text("Top"), 0);
	obs_property_list_add_int(v_align, obs_module_text("Center"), 1);
	obs_property_list_add_int(v_align, obs_module_text("Bottom"), 2);

	obs_properties_add_group(props, "align_group", obs_module_text("Alignment"), OBS_GROUP_NORMAL, align_group);

	obs_properties_t *pos_group = obs_properties_create();
	obs_properties_add_int(pos_group, TEXT_X, obs_module_text("TextX"), -3840, 3840, 1);
	obs_properties_add_int(pos_group, TEXT_Y, obs_module_text("TextY"), -2160, 2160, 1);
	obs_properties_add_int(pos_group, TEXT_WIDTH, obs_module_text("TextWidth"), 1, 3840, 1);
	obs_properties_add_int(pos_group, TEXT_HEIGHT, obs_module_text("TextHeight"), 1, 2160, 1);
	obs_properties_add_bool(pos_group, TEXT_SHOW_BOUNDS, obs_module_text("ShowBounds"));
	obs_properties_add_color_alpha(pos_group, TEXT_BOUNDS_COLOR, obs_module_text("BoundsColor"));
	obs_properties_add_int(pos_group, TEXT_BOUNDS_THICKNESS, obs_module_text("BoundsThickness"), 1, 20, 1);

	obs_properties_add_group(props, "pos_group", obs_module_text("TextPosition"), OBS_GROUP_NORMAL, pos_group);

	// Font Settings
	obs_properties_add_font(props, TEXT_FONT_NAME, obs_module_text("Font"));
	obs_properties_add_int(props, TEXT_FONT_SIZE, obs_module_text("FontSize"), 8, 200, 1);

	obs_property_t *weight = obs_properties_add_list(props, TEXT_FONT_WEIGHT, obs_module_text("FontWeight"),
							 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(weight, obs_module_text("Normal"), 400);
	obs_property_list_add_int(weight, obs_module_text("Bold"), 700);

	// Text Color
	obs_properties_add_color(props, TEXT_COLOR, obs_module_text("TextColor"));

	// Outline
	obs_properties_add_bool(props, TEXT_OUTLINE, obs_module_text("EnableOutline"));
	obs_properties_add_int(props, TEXT_OUTLINE_SIZE, obs_module_text("OutlineSize"), 1, 20, 1);
	obs_properties_add_color(props, TEXT_OUTLINE_COLOR, obs_module_text("OutlineColor"));

	// Shadow
	obs_properties_add_bool(props, TEXT_SHADOW, obs_module_text("EnableShadow"));
	obs_properties_add_int(props, TEXT_SHADOW_OFFSET_X, obs_module_text("ShadowOffsetX"), -50, 50, 1);
	obs_properties_add_int(props, TEXT_SHADOW_OFFSET_Y, obs_module_text("ShadowOffsetY"), -50, 50, 1);
	obs_properties_add_color(props, TEXT_SHADOW_COLOR, obs_module_text("ShadowColor"));

	// Set property callbacks
	obs_property_set_modified_callback(use_folder, use_folder_modified);
	if (data) {
		lyrics_source *ls = (lyrics_source *)data;
		obs_data_t *settings = obs_source_get_settings(ls->source);
		obs_properties_apply_settings(props, settings);
		obs_data_release(settings);
	}

	return props;
}

void lyrics_source_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, USE_FOLDER, false);
	obs_data_set_default_int(settings, TEXT_COLOR, 0xFFFFFFFF);
	obs_data_set_default_int(settings, TEXT_H_ALIGN, 1); // Center
	obs_data_set_default_int(settings, TEXT_V_ALIGN, 2); // Bottom
	obs_data_set_default_int(settings, TEXT_X, 0);
	obs_data_set_default_int(settings, TEXT_Y, 0);
	obs_data_set_default_int(settings, TEXT_WIDTH, 800);
	obs_data_set_default_int(settings, TEXT_HEIGHT, 200);
	obs_data_set_default_bool(settings, TEXT_SHOW_BOUNDS, true);
	obs_data_set_default_int(settings, TEXT_BOUNDS_COLOR, 0x80FFFFFF);
	obs_data_set_default_int(settings, TEXT_BOUNDS_THICKNESS, 2);
	obs_data_set_default_string(settings, TEXT_FONT_NAME, "Arial");
	obs_data_set_default_int(settings, TEXT_FONT_SIZE, 48);
	obs_data_set_default_int(settings, TEXT_FONT_WEIGHT, 400);
	obs_data_set_default_bool(settings, TEXT_OUTLINE, true);
	obs_data_set_default_int(settings, TEXT_OUTLINE_SIZE, 2);
	obs_data_set_default_int(settings, TEXT_OUTLINE_COLOR, 0xFF000000);
	obs_data_set_default_bool(settings, TEXT_SHADOW, false);
	obs_data_set_default_int(settings, TEXT_SHADOW_OFFSET_X, 4);
	obs_data_set_default_int(settings, TEXT_SHADOW_OFFSET_Y, 4);
	obs_data_set_default_int(settings, TEXT_SHADOW_COLOR, 0x80000000);
}
