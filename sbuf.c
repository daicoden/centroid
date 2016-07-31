/* -*- c-file-style: "Ellemtel" -*-

   `sbuf.c' - Underlying i/o buffer for a sockserv (see sbuf.h)
*/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "sbuf.h"

sbuf_t*
sbuf_create(int fd, void* user_data)
{
   sbuf_t* new_sbuf = (sbuf_t*) malloc(sizeof(sbuf_t));

   new_sbuf->fd = fd;
   new_sbuf->user_data = user_data;
   new_sbuf->buffer[0] = '\0';
   new_sbuf->_state = SBUF_EMPTY;
   return new_sbuf;
}

void
sbuf_destroy(sbuf_t* sbuf)
{
   free(sbuf);
}

/*
 *            ---  HANDLE OUTGOING DATA (SEND) ---
 */
static sbuf_state_t
send(sbuf_t* sbuf, int len)
{
   int sent;
   
   sbuf->buffer[len++] = '\n';			/* text mode: add a newline */
   sent = write(sbuf->fd, sbuf->buffer, len);	/* write as much as possible */
   if ( sent == -1 )
   {
      if (errno == EAGAIN || errno == EINTR)
      {
	 sent = 0;
      }
      else
      {
	 return (sbuf->_state = SBUF_ERROR);	/* i/o error */
      }
   }
   if ( sent == len )
   {						/* normal case: */
      sbuf->buffer[0] = '\0';			/* reset the buffer */
      return (sbuf->_state = SBUF_EMPTY);	/* completely sent */
   }
   sbuf->buffer[len-1] = '\0';			/* put term. back */
   if (sent > 0)				/* partially sent */
      memmove(sbuf->buffer, sbuf->buffer + sent, len - sent);
   return (sbuf->_state = SBUF_SENDING);	/* try again later */
}


/*
 *            --- HANDLE INCOMING DATA (RECEIVE) ---
 */
static sbuf_state_t
receive(sbuf_t* sbuf, int len)
{
   int received;
   char* p;

   received = read(sbuf->fd, sbuf->buffer + len, SBUF_SIZE - len);
   if ( received == -1 )
   {
      if ( errno == EAGAIN || errno == EINTR)
      {
	 return (sbuf->_state);
      }
      return (sbuf->_state = SBUF_ERROR);	/* i/o error on read */
   }
   if ( received == 0 )
      return (sbuf->_state = SBUF_EOF);		/* end of file */

   p = sbuf->buffer + received + len - 1;

   if ( *p == '\n' || *p == '\0' )		/* normal case: */
   {						/* strip the newline */
      do { *p-- = '\0'; } while (p >= sbuf->buffer && (*p == '\r'));
      return (sbuf->_state = SBUF_RECEIVED);	/* complete message */
   }

   if (received + len == SBUF_SIZE)
      return (sbuf->_state = SBUF_ERROR);	/* buffer full! */
   
   sbuf->buffer[received+len] = '\0';
   return (sbuf->_state = SBUF_RECEIVING);	/* partial receive */
}

/*
 *            --- Run the state machine. User calls this in a loop.  ---
 */
sbuf_state_t
sbuf_state(sbuf_t* sbuf, int read_set, int write_set)
{
   int len;

   /* === Use strnlen() here for safety, but that is a GNU extension */
   len = strlen(sbuf->buffer);
   if (len >= SBUF_SIZE)
   { errno = EINVAL; return (sbuf->_state = SBUF_ERROR); }

   if (	( sbuf->_state == SBUF_RECEIVED ) ||		/* response required */
      	( sbuf->_state == SBUF_EMPTY && len ) ||	/* user wrote buffer */
	( sbuf->_state == SBUF_SENDING && write_set ) )	/* or continued send */
   {
      return send(sbuf, len);
   }
   if (	( sbuf->_state == SBUF_EMPTY ||
	  sbuf->_state == SBUF_RECEIVING ) && read_set )
   {
      return receive(sbuf, len);
   }
   return (sbuf->_state);				/* No state change */
}
