/*                                             -*- c-file-style: "Ellemtel" -*-

`sockserv.h' - Socket utilities for servers

This file is part of version 1 of the socket client/server utility library.
Read the `License' file for terms of use and distribution.
Copyright 2002, Canada-France-Hawaii Telescope, daprog@cfht.hawaii.edu.

___This header automatically generated from `Index'. Do not edit it here!___ */

/*
 * Documentation is on the Web at http://software.cfht.hawaii.edu/sockio/
 */

#ifndef _INCLUDED_sockserv
#define _INCLUDED_sockserv 1

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "sbuf.h"

typedef void* (*PFCadd) (unsigned char [4]);	/* Add client-info hook */
typedef void (*PFVserv) (void*,char*);		/* Client send/receive hooks */

typedef struct
{
      int port_listen;		/* TCP Port on which the server is listening */
      int fd_listen;		/* File descriptor of the listening socket */
      int fd_max;		/* Highest file descriptor in any mask */
      fd_set readfds;		/* File desc. to watch for incoming data */
      fd_set writefds;		/* File desc. with data waiting to go out  */
      sbuf_t* client_list;	/* Linked list, clients currently connected */
      PFCadd  client_add_hook;	/* Called when a new client is accepted */
      PFVserv client_del_hook;	/* Called when a client disconnects */
      PFVserv client_send_hook;	/* Called whenever possible to send to clnt */
      PFVserv client_recv_hook;	/* Called each time a message is received */
} sockserv_t;

sockserv_t*	sockserv_create(const char* servname);
void		sockserv_destroy(sockserv_t* sockserv); /* (Not included) */
/*
 * Start/stop a socket server, listening on a local TCP port for the service
 * given in "servname".  "servname" must be found in /etc/services (or an
 * equivalent database), or it can be a number of the port to listen on.
 *
 * Returned value is a pointer to structure that manages the socket server.
 * The user should only be interested in the client_*_hook, which can be
 * set to user functions that handle various events during sockserv_run.
 */

#ifdef LINUX
#define SOCK_SIG_IGN SIG_IGN
#else
#define SOCK_SIG_IGN ((void (*)(int))1)
#endif
/*
 * This gets used where SIG_IGN _should_ be.  Unfortunately SIG_IGN is
 * not getting prototyped properly and warnings result.
 */

#define SOCKSERV_POLL 0
#define SOCKSERV_WAIT -1

int		sockserv_run(sockserv_t* sockserv, int timeout_hundredths);
/*
 * This is passed the same pointer returned by a call to sockserv_create,
 * and returns the number of new incoming messages or disconnects.
 *
 * timeout_hundredths must be the maximum time, in hundredths of seconds
 * for which the function should wait for activity from any of the
 * clients.  Use the special values SOCKSERV_POLL and SOCKSERV_WAIT
 * to effect a poll, or wait indefinitely for i/o, respectively.
 *
 * Whenever there is incoming activity, the user function
 *    void client_recv_hook(void* client_info, char* buffer);
 * is called (if set).  This function must process the message in "buffer" and
 * write a response back into the same space pointed to by "buffer" before
 * returning.  The message written back to buffer will be sent to the client.
 * (Thus, if there is no client_recv_hook function, you get an echo server.)
 *
 * Whenever there is an opportunity for additional outgoing messages (other
 * than direct responses to received message, covered above) the user function
 *    void client_send_hook(void* client_info, char* buffer);
 * is called (if set).  The function _may_ write a new, asynchronous message
 * to the client in "buffer".  If the buffer is left alone, nothing happens.
 * It is not possible to send an empty message.  See documentation for details.
 */

#ifdef __cplusplus
}
#endif

#endif /* !_INCLUDED_sockserv */
