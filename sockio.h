/*                                             -*- c-file-style: "Ellemtel" -*-

`sockio.h' - Socket utilities and other file descriptor operations

This file is part of version 1 of the socket client/server utility library.
Read the `License' file for terms of use and distribution.
Copyright 2002, Canada-France-Hawaii Telescope, daprog@cfht.hawaii.edu.

___This header automatically generated from `Index'. Do not edit it here!___ */

#ifndef _INCLUDED_sockio
#define _INCLUDED_sockio 1

#ifndef SBUF_SIZE
#define SBUF_SIZE 32768
#endif

void sock_close_on_exec(int fd);
void sock_non_blocking(int fd);
void sock_lowdelay(int fd);
void sock_linger(int fd, int linger);
void sock_keepalive_and_reuseaddr(int fd);
int  sock_get_tcp_service_port(const char* servname); /* See /etc/services */

#endif /* !_INCLUDED_sockio */
