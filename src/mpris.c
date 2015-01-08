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

	mprisThread = g_thread_new("mpris-listener", startServer, (void *)&mprisData);

	return 0;
}

static int onStop() {
	stopServer();
	g_thread_unref(mprisThread);

	return 0;
}

static int onConnect() {
	DB_artwork_plugin_t *artworkPlugin = (DB_artwork_plugin_t *)mprisData.deadbeef->plug_get_for_id ("artwork");

	if (artworkPlugin != NULL) {
		debug("artwork plugin detected... album art support enabled");
		mprisData.artwork = artworkPlugin;
	} else {
		debug("artwork plugin not detected... album art support disabled");
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
	static int lastState = -1;
	DB_functions_t *deadbeef = mprisData.deadbeef;

	switch (id) {
		case DB_EV_SEEKED:
			debug("DB_EV_SEEKED event received");
			emitSeeked(((ddb_event_playpos_t *) ctx)->playpos);
			break;
		case DB_EV_TRACKINFOCHANGED:
			debug("DB_EV_TRACKINFOCHANGED event received");
			emitMetadataChanged(-1, &mprisData);
			break;
		case DB_EV_SONGSTARTED:
			debug("DB_EV_SONGSTARTED event received");
			emitMetadataChanged(-1, &mprisData);
			emitPlaybackStatusChanged(lastState = OUTPUT_STATE_PLAYING);
			break;
		case DB_EV_PAUSED:
			debug("DB_EV_PAUSED event received");
			debug("PlayPause toggled... last state %d", lastState);
			switch (lastState) {
				case -1:
					emitPlaybackStatusChanged(lastState = deadbeef->get_output()->state());
					break;
				case OUTPUT_STATE_PLAYING:
					emitPlaybackStatusChanged(lastState = OUTPUT_STATE_PAUSED);
					break;
				case OUTPUT_STATE_PAUSED:
					emitPlaybackStatusChanged(lastState = OUTPUT_STATE_PLAYING);
					break;
				default:
					break;
			}
			break;
		case DB_EV_STOP:
			debug("DB_EV_STOP event received");
			emitPlaybackStatusChanged(OUTPUT_STATE_STOPPED);
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
			}
			break;
		default:
			break;
	}

	return 0;
}

DB_misc_t plugin = {
	.plugin.api_vmajor = DB_API_VERSION_MAJOR,\
	.plugin.api_vminor = DB_API_VERSION_MINOR,
	.plugin.type = DB_PLUGIN_MISC,
	.plugin.version_major = 1,
	.plugin.version_minor = 7,
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
	.plugin.configdialog = NULL,
	.plugin.message = handleEvent,
};

DB_plugin_t * mpris_load (DB_functions_t *ddb) {
	debug("Loading...");
	mprisData.deadbeef = ddb;

	return DB_PLUGIN(&plugin);
}
