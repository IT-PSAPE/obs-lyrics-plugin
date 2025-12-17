#include "lyrics-source.h"
#include <obs-module.h>
#include <obs-frontend-api.h>

// Toolbar action callbacks
static void lyrics_next_clicked(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);
	if (!pressed)
		return;

	lyrics_source_next(data);
}

static void lyrics_prev_clicked(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);
	if (!pressed)
		return;

	lyrics_source_previous(data);
}

static void lyrics_stop_clicked(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);
	if (!pressed)
		return;

	lyrics_source_toggle_text(data);
}

// Toolbar actions implementation
struct source_toolbar_action {
	const char *id;
	const char *text;
	const char *icon;
	void (*callback)(void *data);
};

static void next_action(void *data)
{
	lyrics_source_next(data);
}

static void prev_action(void *data)
{
	lyrics_source_previous(data);
}

static void stop_action(void *data)
{
	lyrics_source_toggle_text(data);
}

[[maybe_unused]] static struct source_toolbar_action toolbar_actions[] = {
	{"lyrics.prev", "Previous", "media-skip-backward", prev_action},
	{"lyrics.stop", "Show/Hide", "media-playback-stop", stop_action},
	{"lyrics.next", "Next", "media-skip-forward", next_action}};

// Register toolbar actions with OBS
void lyrics_source_register_toolbar_actions()
{
	// This function would be called from the source info structure
	// to register the toolbar buttons. The actual implementation
	// depends on the OBS API version and frontend API availability
}

// Get toolbar actions for the source
obs_source_info *lyrics_source_get_toolbar_info()
{
	static obs_source_info info = {};

	// Toolbar actions are typically handled through the source_info structure
	// or through frontend API callbacks

	return &info;
}

// Hotkey registration
void lyrics_source_register_hotkeys(void *data, obs_source_t *source)
{
	UNUSED_PARAMETER(data);

	// Register hotkeys for next, previous, and stop
	obs_hotkey_register_source(source, "lyrics.next", obs_module_text("NextLyric"), lyrics_next_clicked, data);

	obs_hotkey_register_source(source, "lyrics.prev", obs_module_text("PreviousLyric"), lyrics_prev_clicked, data);

	obs_hotkey_register_source(source, "lyrics.stop", obs_module_text("ShowHideLyrics"), lyrics_stop_clicked, data);
}
