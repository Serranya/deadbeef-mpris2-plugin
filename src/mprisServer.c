#include <glib.h>
#include <gio/gio.h>

#include "logging.h"

#define BUS_NAME "org.mpris.MediaPlayer2.DeaDBeeF"

static const char xmlForNode[] =
	"<node name='/org/mpris/MediaPlayer2'>"
	"	<interface name='org.mpris.MediaPlayer2'>"
	"		<method name='Raise'/>"
	"		<method name='Quit'/>"
	"		<property access='read'	name='CanQuit'				type='b'/>"
	"		<property access='read'	name='CanRaise'				type='b'/>"
	"		<property access='read'	name='HasTrackList'			type='b'/>"
	"		<property access='read'	name='Identity'				type='s'/>"
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
	"			<arg name='Position'	type='x'/>"
	"		</signal>"
	"		<property access='read'			name='PlaybackStatus'	type='s'/>"
	"		<property access='readwrite'	name='Rate'				type='d'/>"
	"		<property access='read'			name='Metadata'			type='a{sv}'/>"
	"		<property access='readwrite'	name='Volume'			type='d'/>"
	"		<property access='read'			name='Position'			type='x'/>"
	"		<property access='read'			name='MinimumRate'		type='d'/>"
	"		<property access='read'			name='MaximumRate'		type='d'/>"
	"		<property access='read'			name='CanGoNext'		type='b'/>"
	"		<property access='read'			name='CanGoPrevious'	type='b'/>"
	"		<property access='read'			name='CanPlay'			type='b'/>"
	"		<property access='read'			name='CanPause'			type='b' />"
	"		<property access='read'			name='CanSeek'			type='b'/>"
	"		<property access='read'			name='CanControl'		type='b' />"
	"	</interface>"
	"</node>";

static const GDBusInterfaceVTable rootInterfaceVTable = {
	NULL,	//method call
	NULL,	//get property
	NULL	//set property
};

static const GDBusInterfaceVTable playerInterfaceVTable = {
	NULL,	//method call
	NULL,	//get property
	NULL	//set property
};

GMainLoop *loop;

static void onBusAcquiredHandler(GDBusConnection *connection, const char *name, void *userData) {
	debug("Bus accquired");
}

static void onNameAcquiredHandler(GDBusConnection *connection, const char *name, void *userData) {
	debug("name accquired: %s", name);
	int registerIdForRootInterface, registerIdForPlayerInterface;

	registerIdForRootInterface = g_dbus_connection_register_object(connection, "/org/mpris/MediaPlayer2",
			((GDBusNodeInfo *)userData)->interfaces[0], &rootInterfaceVTable, NULL, NULL, NULL);

	registerIdForPlayerInterface = g_dbus_connection_register_object(connection, "/org/mpris/MediaPlayer2",
			((GDBusNodeInfo *)userData)->interfaces[1], &playerInterfaceVTable, NULL, NULL, NULL);
}

static void onConnotConnectToBus(GDBusConnection *connection, const char *name, void *user_data){
	error("cannot connect to bus");
}

void* startServer(void *data) {
	int ownerId;
	GDBusNodeInfo *introspectionData;
	GMainContext *context = g_main_context_new();

	g_main_context_push_thread_default(context);

	introspectionData = g_dbus_node_info_new_for_xml(xmlForNode, NULL);
	ownerId = g_bus_own_name(G_BUS_TYPE_SESSION, BUS_NAME, G_BUS_NAME_OWNER_FLAGS_REPLACE,
			onBusAcquiredHandler, onNameAcquiredHandler, onConnotConnectToBus, introspectionData,NULL);

	loop = g_main_loop_new(context, FALSE);
	g_main_loop_run(loop);

	g_bus_unown_name(ownerId);
	g_dbus_node_info_unref(introspectionData);
	g_main_loop_unref(loop);

	return 0;
}

void stopServer() {
	g_main_loop_quit(loop);
}
