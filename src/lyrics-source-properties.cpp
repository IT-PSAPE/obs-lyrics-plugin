#include "lyrics-source.h"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QWidget>
#include <QFileDialog>
#include <QMainWindow>
#include <QPushButton>
#include <QHBoxLayout>

[[maybe_unused]] static bool background_file_filter(obs_properties_t *props, obs_property_t *property,
						    obs_data_t *settings)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(settings);
	return true;
}

static bool lyrics_folder_clicked(obs_properties_t *props, obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);

	QWidget *parent = (QWidget *)obs_frontend_get_main_window();
	QString dir = QFileDialog::getExistingDirectory(parent, obs_module_text("SelectLyricsFolder"), QString(),
							QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (!dir.isEmpty()) {
		obs_data_t *settings = (obs_data_t *)data;
		obs_data_set_string(settings, LYRICS_FOLDER, dir.toUtf8().constData());
		obs_data_set_bool(settings, USE_FOLDER, true);
		return true;
	}
	return false;
}

static bool lyrics_files_clicked(obs_properties_t *props, obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);

	QWidget *parent = (QWidget *)obs_frontend_get_main_window();
	QStringList files = QFileDialog::getOpenFileNames(parent, obs_module_text("SelectLyricsFiles"), QString(),
							  "Text Files (*.txt);;All Files (*)");

	if (!files.isEmpty()) {
		obs_data_t *settings = (obs_data_t *)data;
		obs_data_array_t *array = obs_data_array_create();

		for (const QString &file : files) {
			obs_data_t *item = obs_data_create();
			obs_data_set_string(item, "value", file.toUtf8().constData());
			obs_data_array_push_back(array, item);
			obs_data_release(item);
		}

		obs_data_set_array(settings, LYRICS_FILES, array);
		obs_data_array_release(array);
		obs_data_set_bool(settings, USE_FOLDER, false);
		return true;
	}
	return false;
}

static bool use_folder_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	bool use_folder = obs_data_get_bool(settings, USE_FOLDER);
	obs_property_t *folder_button = obs_properties_get(props, "folder_button");
	obs_property_t *files_button = obs_properties_get(props, "files_button");
	obs_property_set_visible(folder_button, use_folder);
	obs_property_set_visible(files_button, !use_folder);
	return true;
}

obs_properties_t *lyrics_source_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

	// Background Image
	obs_properties_add_path(props, BACKGROUND_FILE, obs_module_text("BackgroundImage"), OBS_PATH_FILE,
				"Image Files (*.png *.jpg *.jpeg *.gif *.bmp);;All Files (*)", NULL);

	// Lyrics Source Selection
	obs_properties_add_bool(props, USE_FOLDER, obs_module_text("UseFolder"));

	obs_properties_add_button(props, "folder_button", obs_module_text("SelectLyricsFolder"), lyrics_folder_clicked);

	obs_properties_add_button(props, "files_button", obs_module_text("SelectLyricsFiles"), lyrics_files_clicked);

	// Text Position
	obs_property_t *h_align = obs_properties_add_list(props, TEXT_H_ALIGN, obs_module_text("HorizontalAlignment"),
							  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(h_align, obs_module_text("Left"), 0);
	obs_property_list_add_int(h_align, obs_module_text("Center"), 1);
	obs_property_list_add_int(h_align, obs_module_text("Right"), 2);

	obs_property_t *v_align = obs_properties_add_list(props, TEXT_V_ALIGN, obs_module_text("VerticalAlignment"),
							  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(v_align, obs_module_text("Top"), 0);
	obs_property_list_add_int(v_align, obs_module_text("Center"), 1);
	obs_property_list_add_int(v_align, obs_module_text("Bottom"), 2);

	// Text Bounds
	obs_properties_add_int(props, TEXT_WIDTH, obs_module_text("TextWidth"), 100, 3840, 10);
	obs_properties_add_int(props, TEXT_HEIGHT, obs_module_text("TextHeight"), 50, 2160, 10);

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
	obs_property_set_modified_callback(obs_properties_get(props, USE_FOLDER), use_folder_modified);

	return props;
}

void lyrics_source_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, USE_FOLDER, false);
	obs_data_set_default_int(settings, TEXT_COLOR, 0xFFFFFFFF);
	obs_data_set_default_int(settings, TEXT_H_ALIGN, 1); // Center
	obs_data_set_default_int(settings, TEXT_V_ALIGN, 2); // Bottom
	obs_data_set_default_int(settings, TEXT_WIDTH, 800);
	obs_data_set_default_int(settings, TEXT_HEIGHT, 200);
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
