/*

 xmms2-gnome-mediakeys-listener.c
 
 this xmms2 client connects to the gnome settings daemon
 which broadcast out a dbus signal when those prev, play/pause, next keys are hit on your keyboard
 this listens for those signals and tells xmms2 to do something the client lib

 now uses the xmms2 client lib

 2010 rawdod / oneman / drr 

 
 to compile do it like this: 
 
 gcc -Wall -g -ldbus-1 -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include -I/usr/local/include/xmms2 -L/usr/local/lib -lxmmsclient -o xmms2-gnome-mediakeys-listener xmms2-gnome-mediakeys-listener.c


*/

#include <dbus/dbus.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xmmsclient/xmmsclient.h>

void cmd_toggleplay (xmmsc_connection_t *conn);
void cmd_play (xmmsc_connection_t *conn);
void cmd_stop (xmmsc_connection_t *conn);
void cmd_pause (xmmsc_connection_t *conn);
void cmd_next (xmmsc_connection_t *conn);
void cmd_prev (xmmsc_connection_t *conn);
void cmd_jump (xmmsc_connection_t *conn);

  xmmsc_connection_t *connection;
	xmmsc_result_t *result;
	xmmsv_t *return_value;
	const char *err_buf;

static void
do_reljump (xmmsc_connection_t *conn, int where)
{
	xmmsc_result_t *res;
	xmmsv_t *val;
	const char *errmsg;

	res = xmmsc_playlist_set_next_rel (conn, where);
	xmmsc_result_wait (res);

	val = xmmsc_result_get_value (res);
	if (xmmsv_get_error (val, &errmsg)) {
		//print_error ("Couldn't advance in playlist: %s", errmsg);
	}
	xmmsc_result_unref (res);

	res = xmmsc_playback_tickle (conn);
	xmmsc_result_wait (res);

	val = xmmsc_result_get_value (res);
	if (xmmsv_get_error (val, &errmsg)) {
		//print_error ("%s", errmsg);
	}
	xmmsc_result_unref (res);
}


void
cmd_toggleplay (xmmsc_connection_t *conn)
{
	int32_t status;
	xmmsc_result_t *res;
	xmmsv_t *val;
	const char *errmsg;

	res = xmmsc_playback_status (conn);
	xmmsc_result_wait (res);
	val = xmmsc_result_get_value (res);

	if (xmmsv_get_error (val, &errmsg)) {
		//print_error ("Couldn't get playback status: %s", errmsg);
	}

	if (!xmmsv_get_int (val, &status)) {
		//print_error ("Broken resultset");
	}

	if (status == XMMS_PLAYBACK_STATUS_PLAY) {
		cmd_pause (conn);
	} else {
		cmd_play (conn);
	}

	xmmsc_result_unref (res);
}


void
cmd_play (xmmsc_connection_t *conn)
{
	xmmsc_result_t *res;
	xmmsv_t *val;
	const char *errmsg;

	res = xmmsc_playback_start (conn);
	xmmsc_result_wait (res);

	val = xmmsc_result_get_value (res);
	if (xmmsv_get_error (val, &errmsg)) {
	//	print_error ("Couldn't start playback: %s", errmsg);
	}
	xmmsc_result_unref (res);
}

void
cmd_stop (xmmsc_connection_t *conn)
{
	xmmsc_result_t *res;
	xmmsv_t *val;
	const char *errmsg;

	res = xmmsc_playback_stop (conn);
	xmmsc_result_wait (res);

	val = xmmsc_result_get_value (res);
	if (xmmsv_get_error (val, &errmsg)) {
		//print_error ("Couldn't stop playback: %s", errmsg);
	}
	xmmsc_result_unref (res);
}


void
cmd_pause (xmmsc_connection_t *conn)
{
	xmmsc_result_t *res;
	xmmsv_t *val;
	const char *errmsg;

	res = xmmsc_playback_pause (conn);
	xmmsc_result_wait (res);

	val = xmmsc_result_get_value (res);
	if (xmmsv_get_error (val, &errmsg)) {
		//print_error ("Couldn't pause playback: %s", errmsg);
	}
	xmmsc_result_unref (res);
}





void handle_signal(char *command) {

/* printf("%s\n", command); */

if (strcmp("Next", command) == 0) {
	do_reljump (connection, 1);
}

if (strcmp("Previous", command) == 0) {
	do_reljump (connection, -1);
}

if (strcmp("Play", command) == 0) {
	cmd_toggleplay(connection );
}


}

/**
 * Listens for signals on the bus
 */
void receive()
{
   DBusMessage* msg;
   DBusMessageIter args;
   DBusConnection* conn;
   DBusError err;
   int ret;
   char* sigvalue;

   printf("Listening for signals from gnome settings daemon :D \n");

   // initialise the errors
   dbus_error_init(&err);
   
   // connect to the bus and check for errors
   conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
   if (dbus_error_is_set(&err)) { 
      fprintf(stderr, "Connection Error (%s)\n", err.message);
      dbus_error_free(&err); 
   }
   if (NULL == conn) { 
      exit(1);
   }
   
   // request our name on the bus and check for errors
   ret = dbus_bus_request_name(conn, "test.signal.sink", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
   if (dbus_error_is_set(&err)) { 
      fprintf(stderr, "Name Error (%s)\n", err.message);
      dbus_error_free(&err); 
   }
   if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
      exit(1);
   }

   // add a rule for which messages we want to see
   dbus_bus_add_match(conn, "type='signal',interface='org.gnome.SettingsDaemon.MediaKeys'", &err); // see signals from the given interface
   dbus_connection_flush(conn);
   if (dbus_error_is_set(&err)) { 
      fprintf(stderr, "Match Error (%s)\n", err.message);
      exit(1); 
   }
   //printf("Match rule sent\n");

   // loop listening for signals being emmitted
   while (true) {

      // non blocking read of the next available message
      dbus_connection_read_write(conn, 0);
      msg = dbus_connection_pop_message(conn);

      // loop again if we haven't read a message
      if (NULL == msg) { 
         usleep(250000);
         continue;
      }

      // check if the message is a signal from the correct interface and with the correct name
      if (dbus_message_is_signal(msg, "org.gnome.SettingsDaemon.MediaKeys", "MediaPlayerKeyPressed")) {
         
         // read the parameters
         if (!dbus_message_iter_init(msg, &args))
            fprintf(stderr, "Message Has No Parameters\n");
         else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) 
            fprintf(stderr, "Argument is not string!\n"); 
         else
          /* gnome sends out two strings banshee-1 and then the command for whatever reason */
          dbus_message_iter_get_basic(&args, &sigvalue);
          handle_signal(sigvalue);
          dbus_message_iter_next(&args);
          dbus_message_iter_get_basic(&args, &sigvalue);
          handle_signal(sigvalue);

      }

      // free the message
      dbus_message_unref(msg);
   }
}

int main()
{


	connection = xmmsc_init ("gnome-media-keys");
	if (!connection) {
		fprintf (stderr, "OOM!\n");
		exit (EXIT_FAILURE);
	}

	if (!xmmsc_connect (connection, getenv ("XMMS_PATH"))) {
		fprintf (stderr, "Connection failed: %s\n",
		         xmmsc_get_last_error (connection));

		exit (EXIT_FAILURE);
	}

   receive();

   return 0;
}
