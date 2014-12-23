#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <glib.h>
#include <gio/gio.h>
#include <curl/curl.h>
//TODO organize
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "logging.h"
#include "mprisServer.h"

#define BUS_NAME "org.mpris.MediaPlayer2.DeaDBeeF"
#define OBJECT_NAME "/org/mpris/MediaPlayer2"
#define PLAYER_INTERFACE "org.mpris.MediaPlayer2.Player"
#define PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"
#define CURRENT_TRACK -1
#define CACHE_PATH "deadbeef/mpris/cover.png"

void emitSeeked(float);
static char * getCacheDir(void);

static const char xmlForNode[] =
	"<node name='/org/mpris/MediaPlayer2'>"
	"	<interface name='org.mpris.MediaPlayer2'>"
	"		<method name='Raise'/>"
	"		<method name='Quit'/>"
	"		<property access='read'	name='CanQuit'				type='b'/>"
	"		<property access='read'	name='CanRaise'				type='b'/>"
	"		<property access='read'	name='HasTrackList'			type='b'/>"
	"		<property access='read'	name='Identity'				type='s'/>"
	"		<property access='read' name='DesktopEntry'			type='s'/>"
	"		<property access='read'	name='SupportedUriSchemes'	type='as'/>"
	"		<property access='read'	name='SupportedMimeTypes'	type='as'/>"
	"	</interface>"
	"	<interface name='org.mpris.MediaPlayer2.Player'>"
	"		<method name='Next'/>"
	"		<method name='Previous'/>"
	"		<method name='Pause'/>"
	"		<method name='PlayPause'/>"
	"		<method name='Stop'/>"
	"		<method name='Play'/>"
	"		<method name='Seek'>"
	"			<arg name='Offset'		type='x'/>"
	"		</method>"
	"		<method name='SetPosition'>"
	"			<arg name='TrackId'		type='o'/>"
	"			<arg name='Position'	type='x'/>"
	"		</method>"
	"		<method name='OpenUri'>"
	"			<arg name='Uri'			type='s'/>"
	"		</method>"
	"		<signal name='Seeked'>"
	"			<arg name='Position'	type='x' direction='out'/>"
	"		</signal>"
	"		<property access='read'			name='PlaybackStatus'	type='s'/>"
	"		<property access='readwrite'	name='LoopStatus'		type='s'/>"
	"		<property access='readwrite'	name='Rate'				type='d'/>"
	"		<property access='readwrite'	name='Shuffle'			type='b'/>"
	"		<property access='read'			name='Metadata'			type='a{sv}'/>"
	"		<property access='readwrite'	name='Volume'			type='d'/>"
	"		<property access='read'			name='Position'			type='x'>"
	"			<annotation name='org.freedesktop.DBus.Property.EmitsChangedSignal' value='false'/>"
	"		</property>"
	"		<property access='read'			name='MinimumRate'		type='d'/>"
	"		<property access='read'			name='MaximumRate'		type='d'/>"
	"		<property access='read'			name='CanGoNext'		type='b'/>"
	"		<property access='read'			name='CanGoPrevious'	type='b'/>"
	"		<property access='read'			name='CanPlay'			type='b'/>"
	"		<property access='read'			name='CanPause'			type='b'/>"
	"		<property access='read'			name='CanSeek'			type='b'/>"
	"		<property access='read'			name='CanControl'		type='b'>"
	"			<annotation name='org.freedesktop.DBus.Property.EmitsChangedSignal' value='false'/>"
	"		</property>"
	"	</interface>"
	"</node>";

static GDBusConnection *globalConnection = NULL;
static GMainLoop *loop;

static char * writeCover(GdkPixbuf *cover) {
	char *cacheDir = getCacheDir();

	gdk_pixbuf_save(cover, cacheDir, "png", NULL, NULL);
	debug("Artwork saved in %s with length %d", cacheDir, strlen(cacheDir));

	return cacheDir;
}

static void coverartCallback(void *userData) {
	debug("asynchronous loading of albumart successfull");
	emitMetadataChanged(-1, userData);
}

static char * getCacheDir() {
	char *cacheDir;

	char *xdgCacheDir = getenv("XDG_CACHE_HOME");
	if (xdgCacheDir != NULL) {
		cacheDir = malloc(strlen(xdgCacheDir) + 25 + 1); // strlen(xdgCacheDir) + strlen(/deadbeef/mpris/cover.png) + \0

		strcpy(cacheDir, xdgCacheDir);
	} else {
		char *homeDir = getenv("HOME");
		cacheDir = malloc(strlen(homeDir) + 7 + 25 + 1); // strlen(homeDir) + strlen(/.cache) + strlen(/deadbeef/mpris/cover.png) + \0

		strcpy(cacheDir, homeDir);
		strcat(cacheDir, "/.cache");
	}
	strcat(cacheDir, "/deadbeef/mpris");
	if (access(cacheDir, F_OK) != 0) {
		mkdir(cacheDir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); //755
	}
	strcat(cacheDir, "/cover.png");

	return cacheDir;
}

GVariant* getMetadataForTrack(int track_id, struct MprisData *mprisData) {
	int id;
	DB_playItem_t *track = NULL;
	DB_functions_t *deadbeef = mprisData->deadbeef;
	ddb_playlist_t *pl = deadbeef->plt_get_curr();
	GVariant *tmp;
	GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

	if (track_id < 0) {
		track = deadbeef->streamer_get_playing_track();
		id = deadbeef->plt_get_item_idx(pl, track, PL_MAIN);
	} else {
		track = deadbeef->plt_get_item_for_idx(pl, track_id, PL_MAIN);
		id = track_id;
	}
	deadbeef->plt_unref(pl);

	if (track != NULL) {
		char buf[500];
		int buf_size = sizeof(buf);

		deadbeef->pl_lock();

		sprintf(buf, "/org/mpris/MediaPlayer2/DeaDBeeF/playlist/%d", id);
		debug("get Metadata trackid: %s", buf);
		g_variant_builder_add(builder, "{sv}", "mpris:trackid", g_variant_new("o", buf));

		int64_t duration = (int64_t)deadbeef->pl_get_item_duration(track) * 1000000;
		debug("get Metadata duration: %" PRId64, duration);
		g_variant_builder_add(builder, "{sv}", "mpris:length", g_variant_new("x", duration));

		const char *album = deadbeef->pl_find_meta(track, "album");
		debug("get Metadata album: %s", album);
		if (album != NULL) {
			g_variant_builder_add(builder, "{sv}", "xesam:album", g_variant_new("s", album));
		}

		const char *albumArtist = deadbeef->pl_find_meta(track, "albumartist");
		if (albumArtist == NULL)
			albumArtist = deadbeef->pl_find_meta(track, "album artist");
		if (albumArtist == NULL)
			albumArtist = deadbeef->pl_find_meta(track, "band");
		debug("get Metadata albumArtist: %s", albumArtist);
		if (albumArtist != NULL) {
			GVariantBuilder *albumArtistBuilder = g_variant_builder_new(G_VARIANT_TYPE("as"));
			g_variant_builder_add(albumArtistBuilder, "s", albumArtist);
			g_variant_builder_add(builder, "{sv}", "xesam:albumArtist", g_variant_builder_end(albumArtistBuilder));
			g_variant_builder_unref(albumArtistBuilder);
		}

		const char *artist = deadbeef->pl_find_meta(track, "artist");
		debug("get Metadata artist: %s", artist);
		if (artist != NULL) {
			GVariantBuilder *artistBuilder = g_variant_builder_new(G_VARIANT_TYPE("as"));
			g_variant_builder_add(artistBuilder, "s", artist);
			g_variant_builder_add(builder, "{sv}", "xesam:artist", g_variant_builder_end(artistBuilder));
			g_variant_builder_unref(artistBuilder);
		}

		if (mprisData->gui != NULL) {
			debug("getting cover for album %s", album);
			GdkPixbuf *cover = mprisData->gui->get_cover_art_pixbuf(album,
					artist, album, 500, coverartCallback, mprisData);
			if (cover != NULL) {
				char *uri = writeCover(cover);
				char *encodedUri = curl_easy_escape(mprisData->curl, uri, 0);
				char *fullUri = malloc(7 + strlen(encodedUri)); // strlen(file://) + strlen(encodedUri)

				strcpy(fullUri, "file://");
				strcpy(fullUri +7, encodedUri);
				debug("cover for %s ready and written", album);
				g_variant_builder_add(builder, "{sv}", "mpris:artUrl", g_variant_new("s", fullUri));

				free(uri);
				free(fullUri);
				curl_free(encodedUri);
				g_object_unref(cover);
			} else {
				debug("cover for %s not ready", album);
			}
		}

		const char *lyrics = deadbeef->pl_find_meta(track, "lyrics");
		debug("get Metadata lyrics: %s", lyrics);
		if (lyrics != NULL) {
			g_variant_builder_add(builder, "{sv}", "xesam:asText", g_variant_new("s", lyrics));
		}

		const char *comment = deadbeef->pl_find_meta(track, "comment");
		debug("get Metadata comment: %s", comment);
		if (comment != NULL) {
			GVariantBuilder *commentBuilder = g_variant_builder_new(G_VARIANT_TYPE("as"));
			g_variant_builder_add(commentBuilder, "s", comment);
			g_variant_builder_add(builder, "{sv}", "xesam:artist", g_variant_builder_end(commentBuilder));
			g_variant_builder_unref(commentBuilder);
		}

		const char *date = deadbeef->pl_find_meta_raw(track, "year");
		if (date == NULL)
			date = deadbeef->pl_find_meta(track, "date");
		debug("get Metadata contentCreated: %s", date); //TODO format date
		if (date != NULL) {
			g_variant_builder_add(builder, "{sv}", "xesam:contentCreated", g_variant_new("s", date));
		}

		//TODO xesam:genre

		const char *title = deadbeef->pl_find_meta(track, "title");
		debug("get Metadata title: %s", title);
		if (title != NULL) {
			g_variant_builder_add(builder, "{sv}", "xesam:title", g_variant_new("s", title));
		}

		const char *trackNumber = deadbeef->pl_find_meta(track, "track");
		debug("get Metadata trackNumber: %s", trackNumber);
		if (trackNumber != NULL) {
			int trackNumberAsInt = atoi(trackNumber);
			if (trackNumberAsInt > 0) {
				g_variant_builder_add(builder, "{sv}", "xesam:trackNumber", g_variant_new("i", trackNumberAsInt));
			}
		}

		char *uri = curl_easy_escape(mprisData->curl, deadbeef->pl_find_meta(track, ":URI"), 0);
		char *fullUri = malloc(strlen(uri) + 7 + 1); // strlen(uri) + strlen("file://") + \0

		strcpy(fullUri, "file://");
		strcpy(fullUri + 7, uri);
		debug("get Metadata URI: %s", fullUri);
		g_variant_builder_add(builder, "{sv}", "xesam:url", g_variant_new("s", fullUri));

		free(fullUri);
		curl_free(uri);
		deadbeef->pl_unlock();
		deadbeef->pl_item_unref(track);
	}
	tmp = g_variant_builder_end(builder);
	g_variant_builder_unref(builder);

	return tmp;
}

static void onRootMethodCallHandler(GDBusConnection *connection, const char *sender, const char *objectPath,
		const char *interfaceName, const char *methodName, GVariant *parameters, GDBusMethodInvocation *invocation,
		void *userData) {
	debug("Method call on root interface. sender: %s, methodName %s", sender, methodName);
	DB_functions_t *deadbeef = ((struct MprisData *)userData)->deadbeef;

	if (strcmp(methodName, "Quit") == 0) {
		g_dbus_method_invocation_return_value(invocation, NULL);
		deadbeef->sendmessage(DB_EV_TERMINATE, 0, 0, 0);
	} else if (strcmp(methodName, "Raise") == 0) {
		g_dbus_method_invocation_return_value(invocation, NULL);
	} else {
		debug("Error! Unsupported method. %s.%s", interfaceName, methodName);
		g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
				"Method %s.%s not supported", interfaceName, methodName);
	}
}

static GVariant* onRootGetPropertyHandler(GDBusConnection *connection, const char *sender, const char *objectPath,
		const char *interfaceName, const char *propertyName, GError **error, void *userData) {
	debug("Get property call on root interface. sender: %s, propertyName: %s", sender, propertyName);
	GVariant *result = NULL;

	if (strcmp(propertyName, "CanQuit") == 0) {
		result = g_variant_new_boolean(TRUE);
	} else if (strcmp(propertyName, "CanRaise") == 0) {
		result = g_variant_new_boolean(FALSE);
	} else if (strcmp(propertyName, "HasTrackList") == 0) {
		result = g_variant_new_boolean(FALSE);
	} else if (strcmp(propertyName, "Identity") == 0) {
		result = g_variant_new_string("DeaDBeeF");
	} else if (strcmp(propertyName, "DesktopEntry") == 0) {
		result = g_variant_new_string("deadbeef");
	} else if (strcmp(propertyName, "SupportedUriSchemes") == 0) {
		GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
		//TODO find uri schemata
		g_variant_builder_add(builder, "s", "file");
		g_variant_builder_add(builder, "s", "http");
		g_variant_builder_add(builder, "s", "cdda");
		result = g_variant_builder_end(builder);
		g_variant_builder_unref(builder);
	} else if (strcmp(propertyName, "SupportedMimeTypes") == 0){
		GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("as"));

		// MIME types from the .desktop file
		g_variant_builder_add(builder, "s", "audio/aac");
		g_variant_builder_add(builder, "s", "audio/aacp");
		g_variant_builder_add(builder, "s", "audio/x-it");
		g_variant_builder_add(builder, "s", "audio/x-flac");
		g_variant_builder_add(builder, "s", "audio/x-mod");
		g_variant_builder_add(builder, "s", "audio/mpeg");
		g_variant_builder_add(builder, "s", "audio/x-mpeg");
		g_variant_builder_add(builder, "s", "audio/x-mpegurl");
		g_variant_builder_add(builder, "s", "audio/mp3");
		g_variant_builder_add(builder, "s", "audio/prs.sid");
		g_variant_builder_add(builder, "s", "audio/x-scpls");
		g_variant_builder_add(builder, "s", "audio/x-s3m");
		g_variant_builder_add(builder, "s", "application/ogg");
		g_variant_builder_add(builder, "s", "application/x-ogg");
		g_variant_builder_add(builder, "s", "audio/x-vorbis+ogg");
		g_variant_builder_add(builder, "s", "audio/ogg");
		g_variant_builder_add(builder, "s", "audio/wma");
		g_variant_builder_add(builder, "s", "audio/x-xm");
		result = g_variant_builder_end(builder);
	}
	return result;
}

static const GDBusInterfaceVTable rootInterfaceVTable = {
	onRootMethodCallHandler,
	onRootGetPropertyHandler,
	NULL
};

static void onPlayerMethodCallHandler(GDBusConnection *connection, const char *sender, const char *objectPath,
		const char *interfaceName, const char *methodName, GVariant *parameters, GDBusMethodInvocation *invocation,
		void *userData) {
	debug("Method call on Player interface. sender: %s, methodName %s", sender, methodName);
	debug("Parameter signature is %s", g_variant_get_type_string (parameters));
	DB_functions_t *deadbeef = ((struct MprisData *)userData)->deadbeef;

	if (strcmp(methodName, "Next") == 0) {
		g_dbus_method_invocation_return_value(invocation, NULL);
		deadbeef->sendmessage(DB_EV_NEXT, 0, 0, 0);
	} else if (strcmp(methodName, "Previous") == 0) {
		g_dbus_method_invocation_return_value(invocation, NULL);
		deadbeef->sendmessage(DB_EV_PREV, 0, 0, 0);
	} else if (strcmp(methodName, "Pause") == 0) {
		g_dbus_method_invocation_return_value(invocation, NULL);
		deadbeef->sendmessage(DB_EV_PAUSE, 0, 0, 0);
	} else if (strcmp(methodName, "PlayPause") == 0) {
		g_dbus_method_invocation_return_value(invocation, NULL);

		if (deadbeef->get_output()->state() == OUTPUT_STATE_PLAYING) {
			deadbeef->sendmessage(DB_EV_PAUSE, 0, 0, 0);
		} else {
			deadbeef->sendmessage(DB_EV_PLAY_CURRENT, 0, 0, 0);
		}

	} else if (strcmp(methodName, "Stop") == 0) {
		g_dbus_method_invocation_return_value(invocation, NULL);
		deadbeef->sendmessage(DB_EV_STOP, 0, 0, 0);
	} else if (strcmp(methodName, "Play") == 0) {
		if (!deadbeef->get_output()->state() == OUTPUT_STATE_PLAYING)
			deadbeef->sendmessage(DB_EV_PLAY_CURRENT, 0, 0, 0);
		g_dbus_method_invocation_return_value(invocation, NULL);
	} else if (strcmp(methodName, "Seek") == 0) {
		DB_playItem_t *track = deadbeef->streamer_get_playing_track();

		if (track != NULL) {
			float durationInMilliseconds = deadbeef->pl_get_item_duration(track) * 1000.0;
			float positionInMilliseconds= deadbeef->streamer_get_playpos() * 1000.0;
			int64_t offsetInMicroseconds;
			g_variant_get(parameters, "x", &offsetInMicroseconds);
			float offsetInMilliseconds = offsetInMicroseconds / 1000.0;

			float newPositionInMilliseconds = positionInMilliseconds + offsetInMilliseconds;
			if (newPositionInMilliseconds < 0) {
				newPositionInMilliseconds = 0;
			}
			if (newPositionInMilliseconds > durationInMilliseconds) {
				deadbeef->sendmessage(DB_EV_NEXT, 0, 0, 0);
			} else {
				deadbeef->sendmessage(DB_EV_SEEK, 0, newPositionInMilliseconds, 0);
			}

			deadbeef->pl_item_unref(track);
			emitSeeked(newPositionInMilliseconds);
		}
	} else if (strcmp(methodName, "SetPosition") == 0) {
		int64_t position = 0;
		const char *trackId = NULL;

		g_variant_get(parameters, "(&ox)", &trackId, &position);
		debug("Set %s position %d.", trackId, position);

		DB_playItem_t *track = deadbeef->streamer_get_playing_track();
		if (track != NULL) {
			ddb_playlist_t *pl = deadbeef->plt_get_curr();
			int playid = deadbeef->plt_get_item_idx(pl, track, PL_MAIN);
			char buf[200];
			sprintf(buf, "/org/mpris/MediaPlayer2/DeaDBeeF/playlist/%d", playid);
			if (strcmp(buf, trackId) == 0) { //TODO handle different tracks
				deadbeef->sendmessage(DB_EV_SEEK, 0, position / 1000.0, 0);
			}
			deadbeef->pl_item_unref(track);
			deadbeef->plt_unref(pl);
			emitSeeked(deadbeef->streamer_get_playpos() * 1000.0);
		}
		g_dbus_method_invocation_return_value(invocation, NULL);
	} else if (strcmp(methodName, "OpenUri") == 0) {
		const char *uri = NULL;

		g_variant_get(parameters, "(&s)", &uri);
		debug("OpenUri: %s\n", uri);
		//TODO it is probably better to create a new playlist for that
		ddb_playlist_t *pl = deadbeef->plt_get_curr();
		int ret = deadbeef->plt_add_file2(0, pl, uri, NULL, NULL);
		if (ret == 0) {
			ddb_playlist_t *pl = deadbeef->plt_get_curr();
			DB_playItem_t *track = deadbeef->plt_get_last(pl, PL_MAIN);
			int track_id = deadbeef->plt_get_item_idx(pl, track, PL_MAIN);
			deadbeef->plt_unref(pl);
			deadbeef->pl_item_unref(track);
			deadbeef->sendmessage(DB_EV_PLAY_NUM, 0, track_id, 0);
		}
		g_dbus_method_invocation_return_value(invocation, NULL);
	} else {
		debug("Error! Unsupported method. %s.%s", interfaceName, methodName);
		g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
				"Method %s.%s not supported", interfaceName, methodName);
	}
}

static GVariant* onPlayerGetPropertyHandler(GDBusConnection *connection, const char *sender, const char *objectPath,
		const char *interfaceName, const char *propertyName, GError **error, void *userData) {
	debug("Get property call on Player interface. sender: %s, propertyName: %s", sender, propertyName);
	DB_functions_t *deadbeef = ((struct MprisData *)userData)->deadbeef;
	GVariant *result = NULL;

	if (strcmp(propertyName, "PlaybackStatus") == 0) {
		DB_output_t *output = deadbeef->get_output();

		if (output != NULL) {
			switch (output->state()) {
			case OUTPUT_STATE_PLAYING:
				result = g_variant_new_string("Playing");
				break;
			case OUTPUT_STATE_PAUSED:
				result = g_variant_new_string("Paused");
				break;
			case OUTPUT_STATE_STOPPED:
			default:
				result = g_variant_new_string("Stopped");
				break;
			}
		}
	} else if (strcmp(propertyName, "LoopStatus") == 0) {
		int loop = deadbeef->conf_get_int("playback.loop", 0);

		switch (loop) {
		case PLAYBACK_MODE_NOLOOP:
			result = g_variant_new_string("None");
			break;
		case PLAYBACK_MODE_LOOP_ALL:
			result = g_variant_new_string("Playlist");
			break;
		case PLAYBACK_MODE_LOOP_SINGLE:
			result = g_variant_new_string("Track");
			break;
		default:
			result = g_variant_new_string("None");
			break;
		}
	} else if (strcmp(propertyName, "Rate") == 0
			|| strcmp(propertyName, "MaximumRate") == 0
			|| strcmp(propertyName, "MinimumRate") == 0) {
		result = g_variant_new("d", 1.0);
	} else if (strcmp(propertyName, "Shuffle") == 0) {
		if (deadbeef->conf_get_int("playback.order", PLAYBACK_ORDER_LINEAR) == PLAYBACK_ORDER_LINEAR) {
			result = g_variant_new_boolean(FALSE);
		} else {
			result = g_variant_new_boolean(TRUE);
		}
	} else if (strcmp(propertyName, "Metadata") == 0) {
		result = getMetadataForTrack(CURRENT_TRACK, userData);
	} else if (strcmp(propertyName, "Volume") == 0) {
		float volume = (deadbeef->volume_get_db() * 0.02) + 1;

		result = g_variant_new("d", volume);
	} else if (strcmp(propertyName, "Position") == 0) {
		DB_playItem_t *track = deadbeef->streamer_get_playing_track();

		if (track == NULL) {
			result = g_variant_new("x", 0);
		} else {
			float positionInSeconds = deadbeef->streamer_get_playpos();

			result = g_variant_new("x", (uint64_t)(positionInSeconds * 1000000.0));
			deadbeef->pl_item_unref(track);
		}
	} else if (strcmp(propertyName, "CanGoNext") == 0) {
		result = g_variant_new_boolean(TRUE);
	} else if (strcmp(propertyName, "CanGoPrevious") == 0) {
		result = g_variant_new_boolean(TRUE);
	} else if (strcmp(propertyName, "CanPlay") == 0) {
		result = g_variant_new_boolean(TRUE);
	} else if (strcmp(propertyName, "CanPause") == 0) {
		result = g_variant_new_boolean(TRUE);
	} else if (strcmp(propertyName, "CanSeek") == 0) {
		result = g_variant_new_boolean(TRUE);
	} else if (strcmp(propertyName, "CanControl") == 0) {
		result = g_variant_new_boolean(TRUE);
	}
	return result;
}

static int onPlayerSetPropertyHandler(GDBusConnection *connection, const char *sender, const char *objectPath,
		const char *interfaceName, const char *propertyName, GVariant *value, GError **error, gpointer userData) {
	debug("Set property call on Player interface. sender: %s, propertyName: %s", sender, propertyName);
	DB_functions_t *deadbeef = ((struct MprisData *)userData)->deadbeef;

	if (strcmp(propertyName, "LoopStatus") == 0) {
		char *status;
		g_variant_get(value, "s", &status);
		if (status != NULL) {
			debug("status is %s", status);
			if (strcmp(status, "None") == 0) {
				deadbeef->conf_set_int("playback.loop", PLAYBACK_MODE_NOLOOP);
			} else if (strcmp(status, "Playlist") == 0) {
				deadbeef->conf_set_int("playback.loop", PLAYBACK_MODE_LOOP_ALL);
			} else if (strcmp(status, "Track") == 0) {
				deadbeef->conf_set_int("playback.loop", PLAYBACK_MODE_LOOP_SINGLE);
			}

			deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
		}
	} else if (strcmp(propertyName, "Rate") == 0) {
		debug("Setting the rate is not supported");
	} else if (strcmp(propertyName, "Shuffle") == 0) {
		if (g_variant_get_boolean(value)) {
			deadbeef->conf_set_int("playback.order", PLAYBACK_ORDER_RANDOM);
		} else {
			deadbeef->conf_set_int("playback.order", PLAYBACK_ORDER_LINEAR);
		}
		deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
	} else if (strcmp(propertyName, "Volume") == 0) {
		double volume = g_variant_get_double(value);
		if (volume > 1.0) {
			volume = 1.0;
		} else if (volume < 0.0) {
			volume = 0.0;
		}
		float newVolume = ((float)volume * 50) - 50;

		deadbeef->volume_set_db(newVolume);
	}

	return TRUE;
}

static const GDBusInterfaceVTable playerInterfaceVTable = {
	onPlayerMethodCallHandler,
	onPlayerGetPropertyHandler,
	onPlayerSetPropertyHandler
};

//***********
//* SIGNALS *
//***********
void emitVolumeChanged(float volume) {
	GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
	volume = (volume * 0.02) + 1;
	debug("Volume property changed: %f", volume);

	g_variant_builder_add(builder, "{sv}", "Volume", g_variant_new("d", volume));
	GVariant *signal[] = {
		g_variant_new_string(PLAYER_INTERFACE),
		g_variant_builder_end(builder),
		g_variant_new_strv(NULL, 0)
	};

	g_dbus_connection_emit_signal(globalConnection, NULL, OBJECT_NAME, PROPERTIES_INTERFACE, "PropertiesChanged",
			g_variant_new_tuple(signal, 3), NULL);

	g_variant_builder_unref(builder);
}

void emitSeeked(float position) {
	int64_t positionInMicroseconds = position * 1000000.0;
	debug("Seeked to %" PRId64, positionInMicroseconds);

	g_dbus_connection_emit_signal(globalConnection, NULL, OBJECT_NAME, PLAYER_INTERFACE, "Seeked",
			g_variant_new("(x)", positionInMicroseconds), NULL);
}

void emitMetadataChanged(int trackId, struct MprisData *userData) {
	GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);

	g_variant_builder_add(builder, "{sv}", "Metadata", getMetadataForTrack(trackId, userData));
	GVariant *signal[] = {
			g_variant_new_string(PLAYER_INTERFACE),
			g_variant_builder_end(builder),
			g_variant_new_strv(NULL, 0)
	};

	g_dbus_connection_emit_signal(globalConnection, NULL, OBJECT_NAME, PROPERTIES_INTERFACE, "PropertiesChanged",
			g_variant_new_tuple(signal, 3), NULL);

	g_variant_builder_unref(builder);
}

void emitPlaybackStatusChanged(int status) {
	GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);

	switch (status) {
		case OUTPUT_STATE_PLAYING:
			g_variant_builder_add(builder, "{sv}", "PlaybackStatus", g_variant_new_string("Playing"));
			break;
		case OUTPUT_STATE_PAUSED:
			g_variant_builder_add(builder, "{sv}", "PlaybackStatus", g_variant_new_string("Paused"));
			break;
		case OUTPUT_STATE_STOPPED:
		default:
			g_variant_builder_add(builder, "{sv}", "PlaybackStatus", g_variant_new_string("Stopped"));
			break;
	}
	GVariant *signal[] = {
		g_variant_new_string(PLAYER_INTERFACE),
		g_variant_builder_end(builder),
		g_variant_new_strv(NULL, 0)
	};

	g_dbus_connection_emit_signal(globalConnection, NULL, OBJECT_NAME, PROPERTIES_INTERFACE, "PropertiesChanged",
			g_variant_new_tuple(signal, 3), NULL);

	g_variant_builder_unref(builder);
}

static void onBusAcquiredHandler(GDBusConnection *connection, const char *name, void *userData) {
	globalConnection = connection;
	debug("Bus accquired");
}

static void onNameAcquiredHandler(GDBusConnection *connection, const char *name, void *userData) {
	debug("name accquired: %s", name);
	GDBusInterfaceInfo **interfaces = ((struct MprisData*)userData)->gdbusNodeInfo->interfaces;

	debug("Registering" OBJECT_NAME "object...");
	g_dbus_connection_register_object(connection, OBJECT_NAME, interfaces[0], &rootInterfaceVTable, userData, NULL,
			NULL);

	g_dbus_connection_register_object(connection, OBJECT_NAME, interfaces[1], &playerInterfaceVTable, userData, NULL,
			NULL);
}

static void onConnotConnectToBus(GDBusConnection *connection, const char *name, void *user_data){
	error("cannot connect to bus");
}

void* startServer(void *data) {
	int ownerId;
	GMainContext *context = g_main_context_new();
	struct MprisData *mprisData = data;


	g_main_context_push_thread_default(context);

	mprisData->gdbusNodeInfo = g_dbus_node_info_new_for_xml(xmlForNode, NULL);

	ownerId = g_bus_own_name(G_BUS_TYPE_SESSION, BUS_NAME, G_BUS_NAME_OWNER_FLAGS_REPLACE,
			onBusAcquiredHandler, onNameAcquiredHandler, onConnotConnectToBus, (void *)mprisData, NULL);

	loop = g_main_loop_new(context, FALSE);
	g_main_loop_run(loop);

	g_bus_unown_name(ownerId);
	g_dbus_node_info_unref(mprisData->gdbusNodeInfo);
	g_main_loop_unref(loop);

	return 0;
}

void stopServer() {
	debug("Stopping...");
	g_main_loop_quit(loop);
}
