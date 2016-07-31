/*                                             -*- c-file-style: "Ellemtel" -*-

`sockio.c' - Socket utilities and other file descriptor operations

This file is part of version 1 of the socket client/server utility library.
Read the `License' file for terms of use and distribution.
Copyright 2002, Canada-France-Hawaii Telescope, daprog@cfht.hawaii.edu.

___This header automatically generated from `Index'. Do not edit it here!___ */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "sockio.h"

void
sock_close_on_exec(int fd)
{
   /*
    * Set the socket to be closed on an exec (FD_CLOEXEC)
    */
   int flags;

   if ( (flags=fcntl(fd, F_GETFD)) == -1 ||
        fcntl(fd, F_SETFD, flags|FD_CLOEXEC) == -1 )
      perror("warning: fcntl failed");
}

void
sock_non_blocking(int fd)
{
   int flags;

   if ( (flags=fcntl(fd, F_GETFL)) == -1 ||
        fcntl(fd, F_SETFL, flags|O_NONBLOCK) == -1 )
      perror("warning: fcntl failed");
}

void
sock_lowdelay(int fd)
{
#ifdef IPTOS_LOWDELAY
   {
      int lowdelay = IPTOS_LOWDELAY;
      if (setsockopt(fd, IPPROTO_IP, IP_TOS, (char*)&lowdelay,
		     sizeof(lowdelay)) < 0)
	 perror("warning: setsockopt(IPTOS_LOWDELAY) failed");
   }
#else
#warning You do not have the IPTOS_LOWDELAY option.
#endif

#ifdef TCP_NODELAY
   {
      int one = 1;
      if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&one,
		     sizeof(one)) < 0)
	 perror("warning: setsockopt(TCP_NODELAY) failed");
   }
#else
#warning You do not have the TCP_NODELAY option.
#endif
}

void
sock_linger(int fd, int linger)
{
   struct linger l;

   if ((l.l_linger = linger))
      l.l_onoff = 1;
   else
      l.l_onoff = 0;
   
   if (setsockopt(fd, SOL_SOCKET, SO_LINGER,
		  (char*)&l, sizeof(l)) < 0)
      perror("warning: setsockopt(SO_LINGER) failed");
}

void
sock_keepalive_and_reuseaddr(int fd)
{
   int one = 1;

   if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
		  (void *)&one, sizeof(one)) < 0 ||
       setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
		  (void *)&one, sizeof(one)) < 0)
      perror("warning: setsockopt failed");
}

int
sock_get_tcp_service_port(const char* servname)
{
   /*
    * Look up the service name, or treat string as a port number.
    */
   struct servent* service;

   if ( (service = getservbyname(servname, "tcp")) != NULL )
      return htons(service->s_port);

   return atoi(servname);
}
