/*
 ============================================================================
 Name        : mpris.c
 Author      : Peter Lamby
 Version     :
 Copyright   : GPLv2
 Description : DeaDBeeF mpris plugin
 ============================================================================
 */

#define DDB_API_LEVEL 7
#define DDB_WARN_DEPRECATED 1
#include <deadbeef/deadbeef.h>

#include <glib.h>

#include "mprisServer.h"
#include "logging.h"

const GThread *mprisThread;
const DB_functions_t *deadbeef;

static int onStart() {
	mprisThread = g_thread_new(NULL, startServer, (void *)deadbeef);

	return 0;
}

static int onStop() {
	stopServer();

	return 0;
}

static int handleEvent (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {

	switch (id) {
		case DB_EV_SEEKED:
			break;
		case DB_EV_SONGCHANGED:
			break;
		case DB_EV_SONGSTARTED:
			break;
		case DB_EV_PAUSED:
			break;
		case DB_EV_STOP:
			break;
		case DB_EV_TOGGLE_PAUSE:
			break;
		case DB_EV_VOLUMECHANGED:
			emitVolumeChanged(deadbeef->volume_get_db());
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
	.plugin.version_major = 2,
	.plugin.version_minor = 1,
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
	.plugin.website = "TODO",
	.plugin.start = onStart,
	.plugin.stop = onStop,
	.plugin.connect = NULL,
	.plugin.disconnect = NULL,
	.plugin.configdialog = NULL,
	.plugin.message = handleEvent,
};

DB_plugin_t * mpris_load (DB_functions_t *ddb) {
	debug("Loading...");
	deadbeef = ddb;

	return DB_PLUGIN(&plugin);
}
