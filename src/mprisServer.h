#ifndef MPRISSERVER_H_
#define MPRISSERVER_H_

//TODO make optional
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <curl/curl.h>

#define DDB_API_LEVEL 7
#define DDB_WARN_DEPRECATED 1
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>

struct MprisData {
	DB_functions_t *deadbeef;
	ddb_gtkui_t *gui;
	GDBusNodeInfo *gdbusNodeInfo;
	CURL *curl;
};

void* startServer(void*);
void stopServer(void);

void emitVolumeChanged(float);
void emitSeeked(float);
void emitMetadataChanged(int, struct MprisData*);
void emitPlaybackStatusChanged(int);

#endif
