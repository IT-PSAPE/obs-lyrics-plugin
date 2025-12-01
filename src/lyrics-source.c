#include "lyrics-source.h"
#include <util/platform.h>
#include <graphics/graphics.h>
#include <graphics/image-file.h>
#include <obs-module.h>

/* Simple lyrics source plugin for OBS */

struct lyrics_source {
	obs_source_t *source;
	char *text_to_display;
	uint32_t text_color;
};

/* Lifecycle functions */

static const char *lyrics_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "Lyrics Source";
}

static void *lyrics_create(obs_data_t *settings, obs_source_t *source)
{
	struct lyrics_source *context = bzalloc(sizeof(struct lyrics_source));
	context->source = source;
	context->text_to_display = bzalloc(1);
	context->text_color = 0xFFFFFFFF;
	obs_source_update(source, settings);
	return context;
}

static void lyrics_destroy(void *data)
{
	struct lyrics_source *context = data;
	if (context) {
		bfree(context->text_to_display);
		bfree(context);
	}
}

static void lyrics_update(void *data, obs_data_t *settings)
{
	struct lyrics_source *context = data;
	if (!context) return;

	const char *text = obs_data_get_string(settings, "lyrics_text");
	if (text) {
		bfree(context->text_to_display);
		context->text_to_display = bstrdup(text);
	}

	context->text_color = (uint32_t)obs_data_get_int(settings, "text_color");
}

static void lyrics_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	/* Minimal render: no actual graphics drawn in this basic version */
}

static uint32_t lyrics_get_width(void *data)
{
	UNUSED_PARAMETER(data);
	return 1920; /* Default width */
}

static uint32_t lyrics_get_height(void *data)
{
	UNUSED_PARAMETER(data);
	return 1080; /* Default height */
}

static obs_properties_t *lyrics_properties(void *data)
{
	UNUSED_PARAMETER(data);
	obs_properties_t *props = obs_properties_create();
	obs_properties_add_text(props, "lyrics_text", "Lyrics Text", OBS_TEXT_MULTILINE);
	obs_properties_add_color(props, "text_color", "Text Color");
	return props;
}

static void lyrics_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "lyrics_text", "Enter lyrics here");
	obs_data_set_default_int(settings, "text_color", 0xFFFFFFFF);
}

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
