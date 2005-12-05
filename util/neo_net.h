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

NEOERR *ne_net_listen(int port, int *fd);
NEOERR *ne_net_accept(NSOCK **sock, int fd, int data_timeout);
NEOERR *ne_net_connect(NSOCK **sock, const char *host, int port, 
                       int conn_timeout, int data_timeout);
NEOERR *ne_net_close(NSOCK **sock);
NEOERR *ne_net_read(NSOCK *sock, UINT8 *buf, int buflen);
NEOERR *ne_net_read_line(NSOCK *sock, char **buf);
NEOERR *ne_net_read_binary(NSOCK *sock, UINT8 **b, int *blen);
NEOERR *ne_net_read_str_alloc(NSOCK *sock, char **s, int *len);
NEOERR *ne_net_read_int(NSOCK *sock, int *i);
NEOERR *ne_net_write(NSOCK *sock, const char *b, int blen);
NEOERR *ne_net_write_line(NSOCK *sock, const char *s);
NEOERR *ne_net_write_binary(NSOCK *sock, const char *b, int blen);
NEOERR *ne_net_write_str(NSOCK *sock, const char *s);
NEOERR *ne_net_write_int(NSOCK *sock, int i);
NEOERR *ne_net_flush(NSOCK *sock);
void ne_net_shutdown(void);

__END_DECLS

#endif /* __NEO_NET_H_ */
