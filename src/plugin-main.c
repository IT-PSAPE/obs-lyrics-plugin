/*
OBS Lyrics Plugin
Copyright (C) 2024

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>
#include "lyrics-source.h"
#include <obs-frontend-api.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

// Source info structure
static struct obs_source_info lyrics_source_info = {
	.id = "lyrics_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_CONTROLLABLE_MEDIA,
	.get_name = lyrics_source_get_name,
	.create = lyrics_source_create,
	.destroy = lyrics_source_destroy,
	.update = lyrics_source_update,
	.video_render = lyrics_source_render,
	.get_width = lyrics_source_get_width,
	.get_height = lyrics_source_get_height,
	.get_properties = lyrics_source_properties,
	.get_defaults = lyrics_source_get_defaults,
	.media_play_pause = lyrics_source_media_play_pause,
	.media_restart = lyrics_source_media_restart,
	.media_stop = lyrics_source_media_stop,
	.media_next = lyrics_source_media_next,
	.media_previous = lyrics_source_media_previous,
	.media_get_state = lyrics_source_media_get_state,
	.icon_type = OBS_ICON_TYPE_TEXT,
};

// Frontend event callback for toolbar actions
static void on_event(enum obs_frontend_event event, void *data)
{
	UNUSED_PARAMETER(data);

	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		// Register toolbar actions when frontend is ready
		// Note: This requires frontend API support
	}
}

bool obs_module_load(void)
{
	obs_register_source(&lyrics_source_info);

	// Register frontend event handler if available
	if (obs_frontend_get_main_window()) {
		obs_frontend_add_event_callback(on_event, NULL);
	}

	plugin_log(LOG_INFO, "OBS Lyrics Plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(on_event, NULL);
	plugin_log(LOG_INFO, "OBS Lyrics Plugin unloaded");
}
