/*
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef __NEO_SERVER_H_
#define __NEO_SERVER_H_ 1

__BEGIN_DECLS

/* hmm, this callback might need a mechanism for telling the child to
 * end... */
typedef NEOERR *(*NSERVER_REQ_CB)(void *rock, int num, NSOCK *sock);
typedef NEOERR *(*NSERVER_CB)(void *rock, int num);

typedef struct _nserver {
  /* callbacks */
  NSERVER_CB init_cb;
  NSERVER_REQ_CB req_cb;
  NSERVER_CB clean_cb;

  void *data;

  int num_children;
  int num_requests;

  int port;
  int conn_timeout;
  int data_timeout;

  char lockfile[_POSIX_PATH_MAX];

  /* Internal data */
  int accept_lock;
  int server_fd;
} NSERVER;

NEOERR *nserver_proc_start(NSERVER *server, BOOL fork);

__END_DECLS

#endif /* __NEO_SERVER_H_ */
