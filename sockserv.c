/*                                             -*- c-file-style: "Ellemtel" -*-

`sockserv.c' - Socket server event processor (see sockserv.h)
*/

#include <sys/types.h>
#include <sys/time.h>  
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define BACKLOG 35 /* see "man listen" */

#include "sockserv.h"

static int
client_accept(int fd_listen, char remote_ip[4])
/*
 * This should only be called when there is a new client connecting.
 * The client's file descriptor is returned, and the remote IP address
 * is written to remote_ip[] on return.  A -1 is returned on any error.
 */
{
   struct sockaddr_in sa_client; /* incoming connection */
   int sd_client;
   int sa_size = sizeof(sa_client);
   unsigned char* from = (unsigned char*)(&sa_client.sin_addr);

   if ((sd_client = accept(fd_listen, (struct sockaddr*)&sa_client, &sa_size))==-1)
   {
      perror("accept failed");
      return -1;
   }

   /* SO_LINGER is set for fd_listen, but we want to turn it off for the */
   /* client sockets so that a close() of a client cannot hang the server. */
   sock_linger(sd_client, 0);
   sock_lowdelay(sd_client);
   sock_non_blocking(sd_client);
   remote_ip[0] = from[0];
   remote_ip[1] = from[1];
   remote_ip[2] = from[2];
   remote_ip[3] = from[3];
   return sd_client;
}

static int
client_handle(sockserv_t* sockserv, sbuf_t* sbuf)
/*
 * The function manages the file descriptor masks for select and calls
 * the send/receive hooks when appropriate.  It returns:
 *    0 : nothing happened (or ok to sleep)
 *   -1 : error or eof
 *    1 : a new message was received and handled
 */
{
   int rtn = 0;

   for (;;) switch (sbuf_state(sbuf,
			       FD_ISSET(sbuf->fd, &sockserv->readfds),
			       FD_ISSET(sbuf->fd, &sockserv->writefds)))
   {
      case SBUF_SENDING:
      {
	 FD_CLR(sbuf->fd, &sockserv->readfds);
	 FD_SET(sbuf->fd, &sockserv->writefds);
	 return rtn;
      }
      case SBUF_EMPTY:
      {
	 if (sockserv->client_send_hook)
	 {
	    sockserv->client_send_hook(sbuf->user_data, sbuf->buffer);
	    if (sbuf->buffer[0] != '\0')
	       continue;
	 }
	 /* . . . or fall through to receiving case . . . */
      }
      case SBUF_RECEIVING:
      {
	 FD_SET(sbuf->fd, &sockserv->readfds);
	 FD_CLR(sbuf->fd, &sockserv->writefds);
	 return rtn;
      }
      case SBUF_RECEIVED:
      {
	 if (sockserv->client_recv_hook)
	    sockserv->client_recv_hook(sbuf->user_data, sbuf->buffer);
	 if (sbuf->buffer[0] == '\0')	/* recv_hook can cause client to */
	    return -1;			/* be disconnected by writing '\0' */
	 rtn++;
	 continue;
      }
      case SBUF_ERROR:
	 perror("client i/o failure");
      case SBUF_EOF:
      default:
      {
	 FD_CLR(sbuf->fd, &sockserv->readfds);
	 FD_CLR(sbuf->fd, &sockserv->writefds);
	 return -1;
      }
   }
   return -1; /* Not reached */
}

sockserv_t*
sockserv_create(const char* servname)
{
   sockserv_t* rtn;
   struct sockaddr_in sa_server; /* socket address for the port to listen to */

   signal(SIGPIPE, SOCK_SIG_IGN); /* Do this so we get EPIPE (see man pages) */

   rtn = (sockserv_t*)malloc(sizeof(sockserv_t));
   memset(rtn, 0, sizeof(sockserv_t));
   if ((rtn->fd_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))==-1)
   {
      perror("error: socket failed");
      free(rtn);
      return NULL;
   }
   sock_keepalive_and_reuseaddr(rtn->fd_listen);	/* configure a few */
   sock_linger(rtn->fd_listen, 1000);			/* options for the */
   sock_non_blocking(rtn->fd_listen);			/* listening fd */

   /*
    * Look up a port number for the TCP service name.
    */
   if ( (rtn->port_listen = sock_get_tcp_service_port(servname)) == 0 )
   {
      fprintf(stderr, "error: cant find port for service `%s'\n", servname);
      free(rtn);
      return NULL;
   }
   memset((char*)&sa_server, 0, sizeof(sa_server));
   sa_server.sin_family=AF_INET;
   sa_server.sin_addr.s_addr=INADDR_ANY;
   sa_server.sin_port=htons(rtn->port_listen);

   /* Bind and listen to the port. */
   if (bind(rtn->fd_listen, (struct sockaddr*)&sa_server, sizeof(sa_server)) == -1)
   {
      perror("error: bind failed");
      free(rtn);
      return NULL;
   }
   if (listen(rtn->fd_listen, BACKLOG) == -1)
   {
      perror("error: listen failed");
      free(rtn);
      return NULL;
   }

   sock_close_on_exec(rtn->fd_listen);
   sock_lowdelay(rtn->fd_listen);
   rtn->fd_max = rtn->fd_listen;
   FD_ZERO(&rtn->readfds);
   FD_ZERO(&rtn->writefds);
   return rtn;
}

int
sockserv_run(sockserv_t* sockserv, int timeout_hundredths)
{
   sbuf_t* clnt = 0;
   sbuf_t* prev = 0;
   sbuf_t* next = 0;
   int rtn = 0, events = 0;
   struct timeval tv;
   tv.tv_sec = timeout_hundredths / 100;
   tv.tv_usec = ( timeout_hundredths % 100 ) * 10;

   FD_SET(sockserv->fd_listen, &sockserv->readfds);

   rtn = select(sockserv->fd_max+1, &sockserv->readfds, &sockserv->writefds,
		NULL, (timeout_hundredths<0) ? NULL : &tv);
   if (rtn <= 0)
   {
      FD_ZERO(&sockserv->readfds);
      FD_ZERO(&sockserv->writefds);
      events = 0;
   }

   /*
    * Loop through all clients.  This does three things:
    *   1) Handles any pending I/O if the fd_set bits are set for this client.
    *   2) Sets up the fd_set mask again for the next pass.
    *   3) Cleans up any clients that have disconnected.
    */
   clnt = sockserv->client_list;
   while (clnt)
   {
      int result;

      next = clnt->next; /* Save "next" in case clnt needs to be deleted. */
      result = client_handle(sockserv, clnt);
      if (result == -1)
      {
	 if (sockserv->client_del_hook)
	    sockserv->client_del_hook(clnt->user_data, clnt->buffer);
	 close(clnt->fd);
	 if (prev) prev->next = next;
	 else sockserv->client_list = next;
	 sbuf_destroy(clnt);
	 events++;
      }
      else
      {
	 events += result;
	 prev = clnt;
      }
      clnt = next;
   }
   /*
    * Finally, check for new clients trying to connect.
    */
   if (FD_ISSET(sockserv->fd_listen, &sockserv->readfds))
   {
      char remote_ip[4];
      void* cinfo = NULL;
      int fd;

      fd = client_accept(sockserv->fd_listen, remote_ip);
      if (fd != -1)
      {
	 if (!sockserv->client_add_hook ||
	     (cinfo = sockserv->client_add_hook(remote_ip)))
	 {
	    clnt = sbuf_create(fd, cinfo);
	    clnt->next = sockserv->client_list;
	    sockserv->client_list = clnt;
	    if (fd > sockserv->fd_max) sockserv->fd_max = fd;
	    FD_SET(fd, &sockserv->readfds);
	    FD_CLR(fd, &sockserv->writefds);
	 }
	 else close(fd); /* Client rejected by client_add_hook */
      }
   }
   FD_SET(sockserv->fd_listen, &sockserv->readfds); 
   return events;
}
