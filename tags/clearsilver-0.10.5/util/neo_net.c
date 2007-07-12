/*
 * Copyright 2001-2004 Brandon Long
 * All Rights Reserved.
 *
 * ClearSilver Templating System
 *
 * This code is made available under the terms of the ClearSilver License.
 * http://www.clearsilver.net/license.hdf
 *
 */

#include "cs_config.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "neo_misc.h"
#include "neo_err.h"
#include "neo_net.h"
#include "neo_str.h"

static int ShutdownAccept = 0;

void ne_net_shutdown()
{
  ShutdownAccept = 1;
}

/* Server side */
NEOERR *ne_net_listen(int port, int *fd)
{
  int sfd = 0;
  int on = 1;
/*  int flags; */
  struct sockaddr_in serv_addr;

  if ((sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    return nerr_raise_errno(NERR_IO, "Unable to create socket");

  if (setsockopt (sfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
	sizeof(on)) == -1)
  {
    close(sfd);
    return nerr_raise_errno(NERR_IO, "Unable to setsockopt(SO_REUSEADDR)");
  }
   
  if(setsockopt (sfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&on,
	sizeof(on)) == -1) 
  {
    close(sfd);
    return nerr_raise_errno(NERR_IO, "Unable to setsockopt(SO_KEEPALIVE)");
  }
   
  if(setsockopt (sfd, IPPROTO_TCP, TCP_NODELAY, (void *)&on, 
	sizeof(on)) == -1)
  {
    close(sfd);
    return nerr_raise_errno(NERR_IO, "Unable to setsockopt(TCP_NODELAY)");
  }
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  if (bind(sfd,(struct sockaddr *)&(serv_addr),sizeof(struct sockaddr)) == -1)
  {
    close(sfd);
    return nerr_raise_errno(NERR_IO, "Unable to bind to port %d", port);
  }

  /* If set non-block, then we have to use select prior to accept...
   * typically we don't, so we'll leave this out until we have a need
   * for it and then figure out how to work it into the common code */
  /*
  flags = fcntl(sfd, F_GETFL, 0 );
  if (flags == -1)
  {
    close(sfd);
    return nerr_raise_errno(NERR_IO, "Unable to get socket flags for port %d", 
	port);
  }

  if (fcntl(sfd, F_SETFL, flags | O_NDELAY) == -1)
  {
    close(sfd);
    return nerr_raise_errno(NERR_IO, "Unable to set O_NDELAY for port %d", 
	port);
  }
  */

  if (listen(sfd, 100) == -1)
  {
    close(sfd);
    return nerr_raise_errno(NERR_IO, "Unable to listen on port %d", port);
  }
  *fd = sfd;

  return STATUS_OK;
}

NEOERR *ne_net_accept(NSOCK **sock, int sfd, int data_timeout)
{
  NSOCK *my_sock;
  int fd;
  struct sockaddr_in client_addr;
  socklen_t len;

  len = sizeof(struct sockaddr_in);
  while (1)
  {
    fd = accept(sfd, (struct sockaddr *)&client_addr, &len);
    if (fd >= 0) break;
    if (ShutdownAccept || errno != EINTR)
    {
      return nerr_raise_errno(NERR_IO, "accept() returned error");
    }
    if (errno == EINTR)
    {
      ne_warn("accept received EINTR");
    }
  }

  my_sock = (NSOCK *) calloc(1, sizeof(NSOCK));
  if (my_sock == NULL)
  {
    close(fd);
    return nerr_raise(NERR_NOMEM, "Unable to allocate memory for NSOCK");
  }
  my_sock->fd = fd;
  my_sock->remote_ip = ntohl(client_addr.sin_addr.s_addr);
  my_sock->remote_port = ntohs(client_addr.sin_port);
  my_sock->data_timeout = data_timeout;

  *sock = my_sock;

  return STATUS_OK;
}

/* Client side */
NEOERR *ne_net_connect(NSOCK **sock, const char *host, int port, 
                       int conn_timeout, int data_timeout)
{
  struct sockaddr_in serv_addr;
  struct hostent hp;
  struct hostent *php;
  int fd;
  int r = 0, x;
  int flags;
  struct timeval tv;
  fd_set fds;
  int optval;
  socklen_t optlen;
  NSOCK *my_sock;

  /* FIXME: This isn't thread safe... but there's no man entry for the _r
   * version? */

  php = gethostbyname(host);
  if (php == NULL)
  {
    return nerr_raise(NERR_IO, "Host not found: %s", hstrerror(h_errno));
  }
  hp = *php;

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd == -1)
    return nerr_raise_errno(NERR_IO, "Unable to create socket");

  flags = fcntl(fd, F_GETFL, 0 );
  if (flags == -1)
  {
    close(fd);
    return nerr_raise_errno(NERR_IO, "Unable to get socket flags");
  }

  if (fcntl(fd, F_SETFL, flags | O_NDELAY) == -1)
  {
    close(fd);
    return nerr_raise_errno(NERR_IO, "Unable to set O_NDELAY");
  }

  x = 0;
  while (hp.h_addr_list[x] != NULL)
  {
    memcpy(&(serv_addr.sin_addr), hp.h_addr_list[x], sizeof(struct in_addr));
    errno = 0;
    r = connect(fd, (struct sockaddr *) &(serv_addr), sizeof(struct sockaddr_in));
    if (r == 0 || errno == EINPROGRESS) break;
    x++;
  }
  if (r != 0)
  {
    if (errno != EINPROGRESS)
    {
      close(fd);
      return nerr_raise_errno(NERR_IO, "Unable to connect to %s:%d", 
	  host, port);
    }
    tv.tv_sec = conn_timeout;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    r = select(fd+1, NULL, &fds, NULL, &tv);
    if (r == 0)
    {
      close(fd);
      return nerr_raise(NERR_IO, "Connection to %s:%d failed: Timeout", host,
	  port);
    }
    if (r < 0)
    {
      close(fd);
      return nerr_raise_errno(NERR_IO, "Connection to %s:%d failed", host,
	  port);
    }

    optlen = sizeof(optval);

    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) == -1) 
    {
      close(fd);
      return nerr_raise_errno(NERR_IO, 
	  "Unable to getsockopt to determine connection error");
    }

    if (optval)
    {
      close(fd);
      errno = optval;
      return nerr_raise_errno(NERR_IO, "Connection to %s:%d failed", host, 
	  port);
    }
  }
  /* Re-enable blocking... we'll use select on read/write for timeouts
   * anyways, and if we want non-blocking version in the future we'll
   * add a flag or something.
   */
  flags = fcntl(fd, F_GETFL, 0 );
  if (flags == -1)
  {
    close(fd);
    return nerr_raise_errno(NERR_IO, "Unable to get socket flags");
  }

  if (fcntl(fd, F_SETFL, flags & ~O_NDELAY) == -1)
  {
    close(fd);
    return nerr_raise_errno(NERR_IO, "Unable to set O_NDELAY");
  }

  my_sock = (NSOCK *) calloc(1, sizeof(NSOCK));
  if (my_sock == NULL)
  {
    close(fd);
    return nerr_raise(NERR_NOMEM, "Unable to allocate memory for NSOCK");
  }
  my_sock->fd = fd;
  my_sock->remote_ip = ntohl(serv_addr.sin_addr.s_addr);
  my_sock->remote_port = port;
  my_sock->data_timeout = data_timeout;
  my_sock->conn_timeout = conn_timeout;

  *sock = my_sock;

  return STATUS_OK;
}

NEOERR *ne_net_close(NSOCK **sock)
{
  NEOERR *err;

  if (sock == NULL || *sock == NULL) return STATUS_OK;
  err = ne_net_flush(*sock);
  close((*sock)->fd);
  free((*sock));
  *sock = NULL;
  return nerr_pass(err);
}

/* Low level data interface ... we are implementing a buffered stream
 * here, and the fill and flush are designed for that.  More over, our
 * buffered stream assumes a certain type of protocol design where we
 * flush the write buffer before reading... there are possible protocols
 * where this would be grossly inefficient, but I don't expect to use
 * anything like that */

/* Also, an annoyance here... what to do with the EOF case?  Currently,
 * we're just returing with a ol of 0, which means in most cases when
 * calling this we have to check that case as well as standard errors.
 * We could raise an NERR_EOF or something, but that seems like
 * overkill.  We should probably have a ret arg for the case... */
static NEOERR *ne_net_fill(NSOCK *sock)
{
  NEOERR *err;
  struct timeval tv;
  fd_set fds;
  int r;

  /* Ok, we are assuming a model where one side of the connection is the
   * consumer and the other the producer... and then it switches.  So we
   * flush the output buffer (if any) before we read */
  if (sock->ol)
  {
    err = ne_net_flush(sock);
    if (err) return nerr_pass(err);
  }

  /* Ok, we want connections to fail if they don't connect in
   * conn_timeout... but with higher listen queues, the connection could
   * actually connect, but the remote server won't get to it within the
   * conn_timeout, we still want it to fail.  We do that by using the
   * conn_timeout on the first read ... this isn't quite the same as we
   * might actually timeout at almost 2x conn_timeout (if we had to wait
   * for connect and the first read) but its still better then waiting
   * the full data timeout */
  if (sock->conn_timeout)
  {
    tv.tv_sec = sock->conn_timeout;
    sock->conn_timeout = 0;
  }
  else
  {
    tv.tv_sec = sock->data_timeout;
  }
  tv.tv_usec = 0;

  FD_ZERO(&fds);
  FD_SET(sock->fd, &fds);

  r = select(sock->fd+1, &fds, NULL, NULL, &tv);
  if (r == 0)
  {
    return nerr_raise(NERR_IO, "read failed: Timeout");
  }
  if (r < 0)
  {
    return nerr_raise_errno(NERR_IO, "select for read failed");
  }

  sock->ibuf[0] = '\0';
  r = read(sock->fd, sock->ibuf, NET_BUFSIZE);
  if (r < 0)
  {
    return nerr_raise_errno(NERR_IO, "read failed");
  }

  sock->ib = 0;
  sock->il = r;

  return STATUS_OK;
}

NEOERR *ne_net_flush(NSOCK *sock)
{
  fd_set fds;
  struct timeval tv;
  int r;
  int x = 0;

  if (sock->conn_timeout)
  {
    tv.tv_sec = sock->conn_timeout;
  }
  else
  {
    tv.tv_sec = sock->data_timeout;
  }
  tv.tv_usec = 0;

  x = 0;
  while (x < sock->ol)
  {
    FD_ZERO(&fds);
    FD_SET(sock->fd, &fds);

    r = select(sock->fd+1, NULL, &fds, NULL, &tv);
    if (r == 0)
    {
      return nerr_raise(NERR_IO, "write failed: Timeout");
    }
    if (r < 0)
    {
      return nerr_raise_errno(NERR_IO, "select for write failed");
    }

    r = write(sock->fd, sock->obuf + x, sock->ol - x);
    if (r < 0)
    {
      return nerr_raise_errno(NERR_IO, "select for write failed");
    }
    x += r;
  }
  sock->ol = 0;
  return STATUS_OK;
}

/* hmm, we may need something to know how much we've read here... */
NEOERR *ne_net_read(NSOCK *sock, UINT8 *buf, int buflen)
{
  NEOERR *err;
  int x = 0;
  int l;

  x = buflen;
  while (x > 0)
  {
    if (sock->il - sock->ib > 0)
    {
      if (sock->ib + x <= sock->il)
	l = x;
      else
	l = sock->il - sock->ib;

      memcpy(buf + buflen - x, sock->ibuf + sock->ib, l);
      sock->ib += l;
      x -= l;
    }
    else
    {
      err = ne_net_fill(sock);
      if (err) return nerr_pass(err);
      if (sock->il == 0) return STATUS_OK;
    }
  }
  return STATUS_OK;
}

NEOERR *ne_net_read_line(NSOCK *sock, char **buf)
{
  NEOERR *err;
  STRING str;
  UINT8 *nl;
  int l;
  
  string_init(&str);

  while (1)
  {
    if (sock->il - sock->ib > 0)
    {
      nl = memchr(sock->ibuf + sock->ib, '\n', sock->il - sock->ib);
      if (nl == NULL)
      {
	l = sock->il - sock->ib;
	err = string_appendn(&str, (char *)(sock->ibuf + sock->ib), l);
	sock->ib += l;
	if (err) break;
      }
      else
      {
	l = nl - (sock->ibuf + sock->ib);
	err = string_appendn(&str, (char *)(sock->ibuf + sock->ib), l);
	sock->ib += l;
	if (err) break;

	*buf = str.buf;
	return STATUS_OK;
      }
    }
    else
    {
      err = ne_net_fill(sock);
      if (err) break;
      if (sock->il == 0) return STATUS_OK;
    }
  }
  string_clear(&str);
  return nerr_pass(err);
}

static NEOERR *_ne_net_read_int(NSOCK *sock, int *i, char end)
{
  NEOERR *err;
  int x = 0;
  char buf[32];
  char *ep = NULL;

  while (x < sizeof(buf))
  {
    while (sock->il - sock->ib > 0)
    {
      buf[x] = sock->ibuf[sock->ib++];
      if (buf[x] == end) break;
      x++;
      if (x == sizeof(buf)) break;
    }
    if (buf[x] == end) break;
    err = ne_net_fill(sock);
    if (err) return nerr_pass(err);
    if (sock->il == 0) return STATUS_OK;
  }

  if (x == sizeof(buf))
    return nerr_raise(NERR_PARSE, "Format error on stream, expected '%c'", end);

  buf[x] = '\0';
  *i = strtol(buf, &ep, 10);
  if (ep && *ep)
  {
    return nerr_raise(NERR_PARSE, "Format error on stream, expected number followed by '%c'", end);
  }

  return STATUS_OK;
}

NEOERR *ne_net_read_binary(NSOCK *sock, UINT8 **b, int *blen)
{
  NEOERR *err;
  UINT8 *data;
  UINT8 buf[5];
  int l;

  err = _ne_net_read_int(sock, &l, ':');
  if (err) return nerr_pass(err);

  /* Special case to read a NULL */
  if (l < 0)
  {
    *b = NULL;
    if (blen != NULL) *blen = l;
    return STATUS_OK;
  }

  data = (UINT8 *) malloc(l + 1);
  if (data == NULL)
  {
    /* We might want to clear the incoming data here... */
    return nerr_raise(NERR_NOMEM, 
	"Unable to allocate memory for binary data %d" , l);
  }

  err = ne_net_read(sock, data, l);
  if (err) 
  {
    free(data);
    return nerr_pass(err);
  }
  /* check for comma separator */
  err = ne_net_read(sock, buf, 1);
  if (err) 
  {
    free(data);
    return nerr_pass(err);
  }
  if (buf[0] != ',')
  {
    free(data);
    return nerr_raise(NERR_PARSE, "Format error on stream, expected ','");
  }

  *b = data;
  if (blen != NULL) *blen = l;
  return STATUS_OK;
}

NEOERR *ne_net_read_str_alloc(NSOCK *sock, char **s, int *len)
{
  NEOERR *err;
  int l;

  /* just use the binary read and null terminate the string... */
  err = ne_net_read_binary(sock, (UINT8 **)s, &l);
  if (err) return nerr_pass(err);

  if (*s != NULL)
  {
    (*s)[l] = '\0';
  }
  if (len != NULL) *len = l;
  return STATUS_OK;
}

NEOERR *ne_net_read_int(NSOCK *sock, int *i)
{
  return nerr_pass(_ne_net_read_int(sock, i, ','));
}

NEOERR *ne_net_write(NSOCK *sock, const char *b, int blen)
{
  NEOERR *err;
  int x = 0;
  int l;

  x = blen;
  while (x > 0)
  {
    if (sock->ol < NET_BUFSIZE)
    {
      if (sock->ol + x <= NET_BUFSIZE)
      {
	l = x;
      }
      else
      {
	l = NET_BUFSIZE - sock->ol;
      }

      memcpy(sock->obuf + sock->ol, b + blen - x, l);
      sock->ol += l;
      x -= l;
    }
    else
    {
      err = ne_net_flush(sock);
      if (err) return nerr_pass(err);
    }
  }
  return STATUS_OK;
}

NEOERR *ne_net_write_line(NSOCK *sock, const char *s)
{
  NEOERR *err;

  err = ne_net_write(sock, s, strlen(s));
  if (err) return nerr_pass(err);
  err = ne_net_write(sock, "\n", 1);
  if (err) return nerr_pass(err);
  return STATUS_OK;
}

NEOERR *ne_net_write_binary(NSOCK *sock, const char *b, int blen)
{
  NEOERR *err;
  char buf[32];

  if (b == NULL) blen = -1;

  snprintf(buf, sizeof(buf), "%d:", blen);
  err = ne_net_write(sock, buf, strlen(buf));
  if (err) return nerr_pass(err);

  if (blen > 0)
  {
    err = ne_net_write(sock, b, blen);
    if (err) return nerr_pass(err);
  }

  err = ne_net_write(sock, ",", 1);
  if (err) return nerr_pass(err);
  return STATUS_OK;
}

NEOERR *ne_net_write_str(NSOCK *sock, const char *s)
{
  NEOERR *err;

  if (s == NULL)
    err = ne_net_write_binary(sock, s, -1);
  else
    err = ne_net_write_binary(sock, s, strlen(s));
  return nerr_pass(err);
}

NEOERR *ne_net_write_int(NSOCK *sock, int i)
{
  char buf[32];

  snprintf(buf, sizeof(buf), "%d,", i);
  return nerr_pass(ne_net_write(sock, buf, strlen(buf)));
}

