/*
 * Neotonic ClearSilver CGI Kit
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef __NEO_NET_H_
#define __NEO_NET_H_ 1

__BEGIN_DECLS

#define NET_BUFSIZE 4096

typedef struct _neo_sock {
  int fd;
  int data_timeout;
  int conn_timeout;

  UINT32 remote_ip;
  int remote_port;
  
  /* incoming buffer */
  UINT8 ibuf[NET_BUFSIZE];
  int ib;
  int il;

  /* outbound buffer */
  UINT8 obuf[NET_BUFSIZE];
  int ol;
} NSOCK;

NEOERR *net_listen(int port, int *fd);
NEOERR *net_accept(NSOCK **sock, int fd, int data_timeout);
NEOERR *net_connect(NSOCK **sock, char *host, int port, int conn_timeout, int data_timeout);
NEOERR *net_close(NSOCK **sock);
NEOERR *net_read(NSOCK *sock, UINT8 *buf, int buflen);
NEOERR *net_read_line(NSOCK *sock, char **buf);
NEOERR *net_read_binary(NSOCK *sock, UINT8 **b, int *blen);
NEOERR *net_read_str_alloc(NSOCK *sock, char **s, int *len);
NEOERR *net_read_int(NSOCK *sock, int *i);
NEOERR *net_write(NSOCK *sock, UINT8 *b, int blen);
NEOERR *net_write_line(NSOCK *sock, char *s);
NEOERR *net_write_binary(NSOCK *sock, UINT8 *b, int blen);
NEOERR *net_write_str(NSOCK *sock, char *s);
NEOERR *net_write_int(NSOCK *sock, int i);
NEOERR *net_flush(NSOCK *sock);
void net_shutdown(void);

__END_DECLS

#endif /* __NEO_NET_H_ */
