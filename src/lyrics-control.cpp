#include "lyrics-control.hpp"
#include <util/platform.h>
#include <QDir>
#include <QFile>
#include <QTextStream>

LyricsControl::LyricsControl(QWidget *parent) : QDockWidget(parent), ignoreSignals(false)
{
	setWindowTitle("Lyrics Control");
	setObjectName("LyricsControl");

	container = new QWidget(this);
	setWidget(container);

	QVBoxLayout *layout = new QVBoxLayout(container);

	songCombo = new QComboBox();
	connect(songCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LyricsControl::OnSongChanged);
	layout->addWidget(new QLabel("Select Song:"));
	layout->addWidget(songCombo);

	QHBoxLayout *btnLayout = new QHBoxLayout();
	prevBtn = new QPushButton("Previous");
	connect(prevBtn, &QPushButton::clicked, this, &LyricsControl::OnPrevClicked);
	
	nextBtn = new QPushButton("Next");
	connect(nextBtn, &QPushButton::clicked, this, &LyricsControl::OnNextClicked);
	
	btnLayout->addWidget(prevBtn);
	btnLayout->addWidget(nextBtn);
	layout->addLayout(btnLayout);

	toggleBtn = new QPushButton("Hide Lyrics");
	toggleBtn->setCheckable(true);
	connect(toggleBtn, &QPushButton::clicked, this, &LyricsControl::OnToggleClicked);
	layout->addWidget(toggleBtn);

	statusLabel = new QLabel("No Lyrics Source Selected");
	layout->addWidget(statusLabel);

	layout->addStretch();

	/* Hook into OBS frontend */
	auto cb = [](void *data, obs_source_t *source) {
		LyricsControl *ptr = static_cast<LyricsControl*>(data);
		obs_weak_source_t *weak = obs_source_get_weak_source(source);
		ptr->OnSourceSelected(weak);
		obs_weak_source_release(weak);
	};
	
	/* We need to listen to scene item selection, but obs-frontend-api doesn't have a direct "selection changed" callback 
	   that passes the source easily for all cases. 
	   Actually, OBS_FRONTEND_EVENT_SCENE_ITEM_SELECT is triggered.
	   But we can also just query the current selection periodically or hook into the signal if we were inside the UI code.
	   Since we are a plugin, we use obs_frontend_add_event_callback.
	*/
}

LyricsControl::~LyricsControl()
{
}

void LyricsControl::OnSourceSelected(OBSWeakSource source)
{
	weakSource = source;
	UpdateUI();
}

void LyricsControl::UpdateUI()
{
	obs_source_t *source = obs_weak_source_get_source(weakSource);
	if (!source) {
		container->setEnabled(false);
		statusLabel->setText("No Source Selected");
		return;
	}

	const char *id = obs_source_get_id(source);
	if (strcmp(id, "lyrics_source") != 0) {
		obs_source_release(source);
		container->setEnabled(false);
		statusLabel->setText("Selected source is not a Lyrics Source");
		return;
	}

	container->setEnabled(true);
	statusLabel->setText(obs_source_get_name(source));

	obs_data_t *settings = obs_source_get_settings(source);
	const char *folder = obs_data_get_string(settings, "lyrics_folder");
	
	if (!currentFolder.isEmpty() && currentFolder == folder) {
		/* Folder hasn't changed, maybe just update selection */
	} else {
		currentFolder = folder;
		RefreshSongs();
	}

	/* Sync UI with current settings */
	const char *currentFile = obs_data_get_string(settings, "current_song_file");
	if (currentFile) {
		ignoreSignals = true;
		int idx = songCombo->findText(currentFile);
		if (idx != -1) songCombo->setCurrentIndex(idx);
		ignoreSignals = false;
	}

	bool hidden = obs_data_get_bool(settings, "lyrics_hidden");
	toggleBtn->setChecked(hidden);
	toggleBtn->setText(hidden ? "Show Lyrics" : "Hide Lyrics");

	obs_data_release(settings);
	obs_source_release(source);
}

void LyricsControl::RefreshSongs()
{
	ignoreSignals = true;
	songCombo->clear();
	songFiles.clear();

	QDir dir(currentFolder);
	QStringList filters;
	filters << "*.txt";
	dir.setNameFilters(filters);
	
	QFileInfoList list = dir.entryInfoList();
	for (const QFileInfo &info : list) {
		songCombo->addItem(info.fileName());
		songFiles.append(info.fileName());
	}
	ignoreSignals = false;
}

void LyricsControl::OnSongChanged(int index)
{
	if (ignoreSignals) return;
	if (index < 0 || index >= songFiles.size()) return;

	obs_source_t *source = obs_weak_source_get_source(weakSource);
	if (source) {
		obs_data_t *settings = obs_data_create();
		obs_data_set_string(settings, "current_song_file", songFiles[index].toUtf8().constData());
		obs_data_set_int(settings, "current_line_index", 0); /* Reset to start */
		obs_source_update(source, settings);
		obs_data_release(settings);
		obs_source_release(source);
	}
}

void LyricsControl::OnPrevClicked()
{
	obs_source_t *source = obs_weak_source_get_source(weakSource);
	if (source) {
		obs_data_t *settings = obs_source_get_settings(source);
		int cur = obs_data_get_int(settings, "current_line_index");
		obs_data_release(settings);
		
		if (cur > 0) {
			settings = obs_data_create();
			obs_data_set_int(settings, "current_line_index", cur - 1);
			obs_source_update(source, settings);
			obs_data_release(settings);
		}
		obs_source_release(source);
	}
}

void LyricsControl::OnNextClicked()
{
	obs_source_t *source = obs_weak_source_get_source(weakSource);
	if (source) {
		obs_data_t *settings = obs_source_get_settings(source);
		int cur = obs_data_get_int(settings, "current_line_index");
		obs_data_release(settings);
		
		/* We don't know max lines here easily without reading file. 
		   For now, just increment. The source handles bounds checking. */
		settings = obs_data_create();
		obs_data_set_int(settings, "current_line_index", cur + 1);
		obs_source_update(source, settings);
		obs_data_release(settings);
		
		obs_source_release(source);
	}
}

void LyricsControl::OnToggleClicked()
{
	bool hidden = toggleBtn->isChecked();
	toggleBtn->setText(hidden ? "Show Lyrics" : "Hide Lyrics");
	
	obs_source_t *source = obs_weak_source_get_source(weakSource);
	if (source) {
		obs_data_t *settings = obs_data_create();
		obs_data_set_bool(settings, "lyrics_hidden", hidden);
		obs_source_update(source, settings);
		obs_data_release(settings);
		obs_source_release(source);
	}
}

/* Global event callback to detect selection */
static void frontend_event(enum obs_frontend_event event, void *data)
{
	LyricsControl *dock = static_cast<LyricsControl*>(data);
	if (event == OBS_FRONTEND_EVENT_SCENE_ITEM_SELECT) {
		/* Find the first selected item that is a lyrics source */
		OBSWeakSource lyrics_source = nullptr;
		
		obs_frontend_source_list list = {};
		obs_frontend_get_scenes(&list); // This gets scenes, not selection.
		
		/* Getting selection is tricky. We use obs_frontend_get_current_scene -> obs_scene_enum_items */
		obs_source_t *scene_source = obs_frontend_get_current_scene();
		if (scene_source) {
			obs_scene_t *scene = obs_scene_from_source(scene_source);
			if (scene) {
				obs_scene_enum_items(scene, [](obs_scene_t *scene, obs_sceneitem_t *item, void *param) {
					if (obs_sceneitem_selected(item)) {
						obs_source_t *src = obs_sceneitem_get_source(item);
						if (src && strcmp(obs_source_get_id(src), "lyrics_source") == 0) {
							OBSWeakSource *out = (OBSWeakSource*)param;
							*out = obs_source_get_weak_source(src);
							return false; /* Stop */
						}
					}
					return true;
				}, &lyrics_source);
			}
			obs_source_release(scene_source);
		}
		
		dock->OnSourceSelected(lyrics_source);
		if (lyrics_source) obs_weak_source_release(lyrics_source);
	}
}

extern "C" void InitLyricsControl() {
	/* We create the dock and add it to OBS */
	/* Note: obs_frontend_add_dock takes a created dock widget. 
	   However, we want it to be managed by OBS (save state etc).
	   obs_frontend_register_dock is better if available, but for now we use add_dock.
	*/
	
	/* We need to ensure we are on the UI thread or just create it here if this is called from UI init */
	/* plugin_load is called on main thread usually. */
	
	LyricsControl *dock = new LyricsControl();
	obs_frontend_add_dock(dock);
}
