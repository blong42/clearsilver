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

/* Initial version based on multi-proc based server (like apache 1.x)
 *
 * Parts are:
 *   1) server Init
 *   2) sub-proc start
 *   3)   sub-proc init
 *   4)   sub-proc process request
 *   5)   sub-proc cleanup
 *   6) server cleanup
 *
 * Parts 1 & 6 aren't part of the framework, and at this point, I don't
 * think I need to worry about 3 & 5 either, but maybe in the future.
 */

#include "cs_config.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "neo_misc.h"
#include "neo_err.h"
#include "neo_net.h"
#include "ulocks.h"
#include "neo_server.h"

static NEOERR *nserver_child_loop(NSERVER *server, int num)
{
  NEOERR *err = STATUS_OK, *clean_err;
  int loop = 0;
  NSOCK *child_sock;

  if (server->init_cb)
  {
    err = server->init_cb(server->data, num);
    if (err) return nerr_pass(err);
  }

  while (loop++ < server->num_requests)
  {
    err = fLock(server->accept_lock);
    if (err) break;
    err = ne_net_accept(&child_sock, server->server_fd, server->data_timeout);
    fUnlock(server->accept_lock);
    if (err) break;
    err = server->req_cb(server->data, num, child_sock);
    if (err)
    {
      ne_net_close(&child_sock);
    }
    else
    {
      err = ne_net_close(&child_sock);
    }
    nerr_log_error(err);
    nerr_ignore(&err);
  }
  ne_warn("nserver child loop handled %d connections", loop-1);

  if (server->clean_cb)
  {
    clean_err = server->clean_cb(server->data, num);
    if (clean_err) 
    {
      nerr_log_error(clean_err);
      nerr_ignore(&clean_err);
    }
  }

  return nerr_pass(err);
}

static void ignore_pipe(void)
{
  struct sigaction sa;


  memset(&sa, 0, sizeof(struct sigaction));

  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGPIPE, &sa, NULL);
}

/* Handle shutdown by accepting a TERM signal and then passing it to our
 * program group */
static int ShutdownPending = 0;

static void sig_term(int sig)
{
  ShutdownPending = 1;
  ne_net_shutdown();
}

static void setup_term(void)
{
  struct sigaction sa;


  memset(&sa, 0, sizeof(struct sigaction));

  sa.sa_handler = sig_term;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGTERM, &sa, NULL);
}

NEOERR *nserver_proc_start(NSERVER *server, BOOL debug)
{
  NEOERR *err;

  if (server->req_cb == NULL)
    return nerr_raise(NERR_ASSERT, "nserver requires a request callback");

  ignore_pipe();

  setup_term();

  ShutdownPending = 0;

  err = fFind(&(server->accept_lock), server->lockfile);
  if (err && nerr_handle(&err, NERR_NOT_FOUND))
  {
    err = fCreate(&(server->accept_lock), server->lockfile);
  }
  if (err) return nerr_pass(err);

  do
  {
    err = ne_net_listen(server->port, &(server->server_fd));
    if (err) break;

    if (debug == TRUE)
    {
      err = nserver_child_loop(server, 0);
      break;
    }
    else
    {
      /* create children and restart them as necessary */
      pid_t child;
      int count, status;

      for (count = 0; count < server->num_children; count++)
      {
	child = fork();
	if (child == -1)
	{
	  err = nerr_raise_errno(NERR_SYSTEM, "Unable to fork child");
	  break;
	}
	if (!child)
	{
	  err = nserver_child_loop(server, count);
	  if (err) exit(-1);
	  exit(0);
	}
	ne_warn("Starting child pid %d", child);
      }
      if (count < server->num_children) break;
      while (!ShutdownPending)
      {
	child = wait3(&status, 0, NULL);
	if (child == -1)
	{
	   ne_warn("wait3 failed [%d] %s", errno, strerror(errno));
	   continue;
	}
	if (WIFSTOPPED(status))
	{
	  ne_warn("pid %d stopped on signal %d", child, WSTOPSIG(status));
	  continue;
	}
	if (WIFEXITED(status))
	{
	  /* at some point, we might do something here with the
	   * particular exit value */
	  ne_warn("pid %d exited, returned %d", child, WEXITSTATUS(status));
	}
	else if (WIFSIGNALED(status))
	{
	  ne_warn("pid %d exited on signal %d", child, WTERMSIG(status));
	}
	count++;

	child = fork();
	if (child == -1)
	{
	  err = nerr_raise_errno(NERR_SYSTEM, "Unable to fork child");
	  break;
	}
	if (!child)
	{
	  err = nserver_child_loop(server, count);
	  if (err) exit(-1);
	  exit(0);
	}
	ne_warn("Starting child pid %d", child);
      }
      /* At some point, we might want to actually maintain information
       * on our children, and then we can be more specific here in terms
       * of making sure they all shutdown... for now, fergitaboutit */
      if (ShutdownPending)
      {
	killpg(0, SIGTERM);
      }
    }
  }
  while (0);

  fDestroy(server->accept_lock);
  return nerr_pass(err);
}
