#ifndef MPRISSERVER_H_
#define MPRISSERVER_H_

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#define DDB_API_LEVEL 7
#define DDB_WARN_DEPRECATED 1
#include <deadbeef/deadbeef.h>
#include "artwork.h"

struct MprisData {
	DB_functions_t *deadbeef;
	DB_artwork_plugin_t *artwork;
	GDBusNodeInfo *gdbusNodeInfo;
};

void* startServer(void*);
void stopServer(void);

void emitVolumeChanged(float);
void emitSeeked(float);
void emitMetadataChanged(int, struct MprisData*);
void emitPlaybackStatusChanged(int, struct MprisData*);
void emitLoopStatusChanged(int);
void emitShuffleStatusChanged(int);
void emitCanGoChanged(struct MprisData *);

#endif
