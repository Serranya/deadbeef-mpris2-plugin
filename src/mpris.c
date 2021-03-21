#include <glib.h>

#include "mprisServer.h"
#include "logging.h"

static GThread *mprisThread;
static struct MprisData mprisData;

static int oldLoopStatus = -1;
static int oldShuffleStatus = -1;

static int onStart() {
	oldLoopStatus = mprisData.deadbeef->conf_get_int("playback.loop", 0);
	oldShuffleStatus = mprisData.deadbeef->conf_get_int("playback.order", PLAYBACK_ORDER_LINEAR);
	mprisData.previousAction = mprisData.deadbeef->conf_get_int(SETTING_PREVIOUS_ACTION, PREVIOUS_ACTION_PREV_OR_RESTART);

#if (GLIB_MAJOR_VERSION <= 2 && GLIB_MINOR_VERSION < 32)
	mprisThread = g_thread_create(startServer, (void *)&mprisData, TRUE, NULL);
#else
	mprisThread = g_thread_new("mpris-listener", startServer, (void *)&mprisData);
#endif
	return 0;
}

static int onStop() {
	stopServer();

#if (GLIB_MAJOR_VERSION <= 2 && GLIB_MINOR_VERSION < 32)
	g_thread_join(mprisThread);
#else
	g_thread_unref(mprisThread);
#endif

	return 0;
}

static int onConnect() {
	mprisData.artwork = NULL;
	mprisData.prevOrRestart = NULL;

	DB_artwork_plugin_t *artworkPlugin = (DB_artwork_plugin_t *)mprisData.deadbeef->plug_get_for_id ("artwork");

	if (artworkPlugin != NULL) {
		debug("artwork plugin detected... album art support enabled");
		mprisData.artwork = artworkPlugin;
	} else {
		debug("artwork plugin not detected... album art support disabled");
	}

	DB_plugin_t *hotkeysPlugin = mprisData.deadbeef->plug_get_for_id ("hotkeys");

	if (hotkeysPlugin != NULL) {
		debug("hotkeys plugin detected...");

		DB_plugin_action_t *dbaction;

		for (dbaction = hotkeysPlugin->get_actions (NULL); dbaction; dbaction = dbaction->next) {
			if (strcmp(dbaction->name, "prev_or_restart") == 0) {
				debug("prev_or_restart command detected... previous or restart support enabled");
				mprisData.prevOrRestart = dbaction;
				break;
			}
		}

		if (mprisData.prevOrRestart == NULL) {
			debug("prev_or_restart command not detected... previous or restart support disabled");
		}
	} else {
		debug("hotkeys plugin not detected... previous or restart support disabled");
	}

	return 0;
}

//***********************
//* Handels signals for *
//* - Playback status   *
//* - Metadata          *
//* - Volume            *
//* - Seeked            *
//* - Loop status       *
//* - Shuffle status    *
//***********************
static int handleEvent (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
	DB_functions_t *deadbeef = mprisData.deadbeef;

	switch (id) {
		case DB_EV_SEEKED:
			debug("DB_EV_SEEKED event received");
			emitSeeked(((ddb_event_playpos_t *) ctx)->playpos);
			break;
		case DB_EV_TRACKINFOCHANGED:
			debug("DB_EV_TRACKINFOCHANGED event received");
			emitMetadataChanged(-1, &mprisData);
			emitCanGoChanged(&mprisData);
			emitSeeked(deadbeef->streamer_get_playpos());
			break;
		case DB_EV_SELCHANGED:
		case DB_EV_PLAYLISTSWITCHED:
			emitCanGoChanged(&mprisData);
			break;
		case DB_EV_SONGSTARTED:
			debug("DB_EV_SONGSTARTED event received");
			emitMetadataChanged(-1, &mprisData);
			emitPlaybackStatusChanged(OUTPUT_STATE_PLAYING, &mprisData);
			break;
		case DB_EV_PAUSED:
			debug("DB_EV_PAUSED event received");
			emitPlaybackStatusChanged(p1 ? OUTPUT_STATE_PAUSED : OUTPUT_STATE_PLAYING, &mprisData);
			break;
		case DB_EV_STOP:
			debug("DB_EV_STOP event received");
			emitPlaybackStatusChanged(OUTPUT_STATE_STOPPED, &mprisData);
			break;
		case DB_EV_VOLUMECHANGED:
			debug("DB_EV_VOLUMECHANGED event received");
			emitVolumeChanged(deadbeef->volume_get_db());
			break;
		case DB_EV_CONFIGCHANGED:
			debug("DB_EV_CONFIGCHANGED event received");
			if (oldShuffleStatus != -1) {
				int newLoopStatus = mprisData.deadbeef->conf_get_int("playback.loop", PLAYBACK_MODE_LOOP_ALL);
				int newShuffleStatus = mprisData.deadbeef->conf_get_int("playback.order", PLAYBACK_ORDER_LINEAR);

				if (newLoopStatus != oldLoopStatus) {
					debug("LoopStatus changed %d", newLoopStatus);
					emitLoopStatusChanged(oldLoopStatus = newLoopStatus);
				} if (newShuffleStatus != oldShuffleStatus) {
					debug("ShuffleStatus changed %d", newShuffleStatus);
					emitShuffleStatusChanged(oldShuffleStatus = newShuffleStatus);
				}

				mprisData.previousAction = mprisData.deadbeef->conf_get_int(SETTING_PREVIOUS_ACTION, PREVIOUS_ACTION_PREV_OR_RESTART);
			}
			break;
		default:
			break;
	}

	return 0;
}

#define STR(x) #x
#define XSTR(x) STR(x)

static const char settings_dlg[] =
	"property \"\\\"Previous\\\" action behavior\" select[2] " SETTING_PREVIOUS_ACTION " " XSTR(PREVIOUS_ACTION_PREV_OR_RESTART) " \"Previous\" \"Previous or restart current track\";";


DB_misc_t plugin = {
	.plugin.api_vmajor = DB_API_VERSION_MAJOR,\
	.plugin.api_vminor = DB_API_VERSION_MINOR,
	.plugin.type = DB_PLUGIN_MISC,
	.plugin.version_major = PLUGIN_VERSION_MAJOR,
	.plugin.version_minor = PLUGIN_VERSION_MINOR,
	.plugin.id = "mpris",
	.plugin.name ="MPRISv2 plugin",
	.plugin.descr = "Communicate with other applications using D-Bus.",
	.plugin.copyright =
			"Copyright (C) 2014 Peter Lamby <peterlamby@web.de>\n"
			"\n"
			"This program is free software; you can redistribute it and/or\n"
			"modify it under the terms of the GNU General Public License\n"
			"as published by the Free Software Foundation; either version 2\n"
			"of the License, or (at your option) any later version.\n"
			"\n"
			"This program is distributed in the hope that it will be useful,\n"
			"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
			"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
			"GNU General Public License for more details.\n"
			"\n"
			"You should have received a copy of the GNU General Public License\n"
			"along with this program; if not, write to the Free Software\n"
			"Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
	,
	.plugin.website = "https://github.com/Serranya/deadbeef-mpris2-plugin",
	.plugin.start = onStart,
	.plugin.stop = onStop,
	.plugin.connect = onConnect,
	.plugin.disconnect = NULL,
	.plugin.configdialog = settings_dlg,
	.plugin.message = handleEvent,
};

DB_plugin_t * mpris_load (DB_functions_t *ddb) {
	debug("Loading...");
	mprisData.deadbeef = ddb;

	return DB_PLUGIN(&plugin);
}
