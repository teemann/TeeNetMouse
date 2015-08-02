#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <glib.h>

#include "os.h"
#ifdef _WIN32
#include <windows.h>
#endif

void incoming(GThreadedSocketService* serv, GSocketConnection* conn, GObject* src, gpointer data);
gpointer input(gpointer data);
void getCurD(POINT* p);
gpointer mouseThr(gpointer data);

int nconn = 0;
int todo = 0;
int rdy = 0;

int dx = 0, dy = 0;

G_LOCK_DEFINE(nconn);
G_LOCK_DEFINE(delta);

GMainLoop* loop;

int main() {
	g_print("Test123\n");
	GSocketService* serv = g_threaded_socket_service_new(25);
	g_socket_listener_add_inet_port((GSocketListener*)serv, 31056, 0, 0);
	g_signal_connect(serv, "run", G_CALLBACK(&incoming), 0);
	g_socket_service_start(serv);

	loop = g_main_loop_new(0, 1);

	g_thread_new("mouse", (GThreadFunc)&mouseThr, 0);
	GThread* inp = g_thread_new("input", (GThreadFunc)&input, 0);

	g_main_loop_run(loop);

	g_main_loop_unref(loop);
	g_socket_service_stop(serv);
	g_socket_listener_close((GSocketListener*)serv);
	g_thread_unref(inp);

	return 0;
}

void incoming(GThreadedSocketService* serv, GSocketConnection* conn, GObject* src, gpointer data) {
	POINT p;
	GSocket* sock = g_socket_connection_get_socket(conn);
	char* buffer = malloc(1024);
	int i = 0;
	G_LOCK(nconn);
	nconn++;
	G_UNLOCK(nconn);
	g_print("Connection\n");
	while (!g_socket_is_closed(sock)) {
		getCurD(&p);
		sprintf(buffer, "m,%d,%d\r\n", p.x, p.y);
		if (g_socket_send(sock, buffer, strlen(buffer), 0, 0) == -1)
			break;
		i++;
		i %= 100000;
	}
	g_print("Close\n");
	G_LOCK(nconn);
	nconn--;
	G_UNLOCK(nconn);
	free(buffer);
}

gpointer mouseThr(gpointer data) {
	int wid = GetSystemMetrics(SM_CXSCREEN);
	int hei = GetSystemMetrics(SM_CYSCREEN);
	int x = wid / 2;
	int y = hei / 2;
	POINT p;
	while (g_main_loop_is_running(loop)) {
		if(nconn > 0)
			SetCursorPos(x, y);
		g_usleep(1000 * 20);
		GetCursorPos(&p);
		G_LOCK(delta);
		dx = p.x - x;
		dy = p.y - y;
		todo = nconn;
		rdy = 1;
		G_UNLOCK(delta);
		while (todo != 0) {
			g_usleep(5);
		}
		G_LOCK(delta);
		rdy = 0;
		G_UNLOCK(delta);
	}
	return 0;
}

gpointer input(gpointer data) {
	char* buf = malloc(1024);
	while (1) {
		fgets(buf, 1024, stdin);
		if (strstr(buf, "quit") == buf) {
			g_print("Quitting...\n");
			free(buf);
			g_main_loop_quit(loop);
			return 0;
		}
	}
}

void getCurD(POINT* p) {
	while (!rdy)
		g_usleep(5);
	p->x = dx;
	p->y = dy;
	G_LOCK(delta);
	todo--;
	G_UNLOCK(delta);
}