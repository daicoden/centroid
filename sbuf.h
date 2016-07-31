/*                                             -*- c-file-style: "Ellemtel" -*-

`sbuf.h' - Underlying i/o buffer for a sockserv

This file is part of version 1 of the socket client/server utility library.
Read the `License' file for terms of use and distribution.
Copyright 2002, Canada-France-Hawaii Telescope, daprog@cfht.hawaii.edu.

___This header automatically generated from `Index'. Do not edit it here!___ */

/*
 * Documentation is on the Web at http://software.cfht.hawaii.edu/sockio/
 */

#ifndef _INCLUDED_sbuf
#define _INCLUDED_sbuf 1

#include "sockio.h"

typedef enum
{
   SBUF_EMPTY = 0,		/* Not in use */
   SBUF_RECEIVING,		/* Partial incoming message in buffer */
   SBUF_RECEIVED,		/* Complete message, response required */
   SBUF_SENDING,		/* Partially sent message in buffer */
   SBUF_EOF,			/* End-of-file condition on file descriptor */
   SBUF_ERROR 			/* Error condition on file descriptor */
} sbuf_state_t;

typedef struct sbuf_tag
{
      struct sbuf_tag* next;	/* For use in making a linked list */
      int fd;			/* File descriptor to read/write */
      void* user_data;		/* Optional pointer to user data */
      char buffer[SBUF_SIZE];	/* Both send and receive use this space */
      sbuf_state_t _state;	/* Internal use. User calls sbuf_state(). */
} sbuf_t;

sbuf_t*		sbuf_create(int fd, void* user_data);
/*
 * Allocate and return a pointer to new sbuf_t structure.  "fd" must be a
 * valid file descriptor corresponding to an already opened socket.
 * "user_data" is an arbitrary pointer which will be saved in the user_data
 * field of the structure.  It's also possible to assign this later directly.
 */

void		sbuf_destroy(sbuf_t* sbuf);
/*
 * Free the data allocated by sbuf_create.  Note that it is the caller's
 * responsibility to free any user_data associated with the buffer.
 */

sbuf_state_t	sbuf_state(sbuf_t* sbuf, int read_set, int write_set);
/*
 * This is here all socket i/o actually happens.  Call this in a loop.
 *
 * - When the state returned is SBUF_EMPTY, the caller may write an outgoing
 * message directly to sbuf->buffer and continue calling sbuf_state().
 *
 * - When the state returned is SBUF_RECEIVED, the caller _must_ process the
 * incoming message _and_ write an outgoing response to the same buffer before
 * calling sbuf_state() again.  I you do not, the sbuf acts as an echo server.
 *
 * Let sockserv.c run the sbuf_t for you.  For details, see the documentation.
 */

#endif /* !_INCLUDED_sbuf */
