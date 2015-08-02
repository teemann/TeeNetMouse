#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <glib.h>

void incoming(GThreadedSocketService* serv, GSocketConnection* conn, GObject* src, gpointer data);
gpointer input(gpointer data);

GMainLoop* loop;

int main() {
	g_print("Test123\n");
	GSocketService* serv = g_threaded_socket_service_new(25);
	g_socket_listener_add_inet_port(serv, 31056, 0, 0);
	g_signal_connect(serv, "run", G_CALLBACK(&incoming), 0);
	g_socket_service_start(serv);

	GThread* inp = g_thread_new("input", (GThreadFunc)&input, 0);

	loop = g_main_loop_new(0, 1);
	g_main_loop_run(loop);

	g_main_loop_unref(loop);
	g_socket_service_stop(serv);
	g_thread_unref(inp);

	return 0;
}

void incoming(GThreadedSocketService* serv, GSocketConnection* conn, GObject* src, gpointer data) {
	GSocket* sock = g_socket_connection_get_socket(conn);
	char* buffer = malloc(1024);
	strcpy(buffer, "Hallo, das ist ein Test.\r\n");
	g_socket_send(sock, buffer, strlen(buffer), 0, 0);
	free(buffer);
}

gpointer input(gpointer data) {
	char* buf = malloc(1024);
	while (1) {
		fgets(buf, 1024, stdin);
		if (!strcmp(buf, "quit")) {
			g_print("Quitting...\n");
			free(buf);
			g_main_loop_quit(loop);
			return 0;
		}
	}
	free(buf);
}