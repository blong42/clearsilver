
#include "cs_config.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/neo_net.h"
#include "util/ulist.h"
#include "util/neo_rand.h"

#define TEST_PORT 46032
#define COUNT 10000

typedef struct _rand_thing {
  int is_num;
  int n;
  char *s;
} RAND_THING;


NEOERR *client_proc(int port, ULIST *stuff)
{
  NEOERR *err;
  NSOCK *nsock;
  int x;
  RAND_THING *thing;

  sleep(1);
  ne_warn("[c] Connecting to port %d", port);
  err = ne_net_connect(&nsock, "localhost", port, 10, 10);
  if (err) return nerr_pass(err);

  ne_warn("[c] Connected.");

  do
  {
    err = ne_net_write_int(nsock, uListLength(stuff));
    if (err) break;

    for (x = 0; x < uListLength(stuff); x++)
    {
      err = uListGet(stuff, x, (void *)&thing);
      if (err) break;
      if (thing->is_num)
      {
	err = ne_net_write_int(nsock, thing->n);
	/* ne_warn("[c] Sending %d", thing->n); */
      }
      else
      {
	err = ne_net_write_str(nsock, thing->s);
	/* ne_warn("[c] Sending %s", thing->s); */
      }
      if (err) break;
    }
  } while (0);

  ne_net_close(&nsock);
  return nerr_pass(err);
}

NEOERR *server_proc(int port, ULIST *stuff)
{
  NEOERR *err;
  int server;
  NSOCK *nsock;
  int x, i;
  RAND_THING *thing;
  char *s;

  ne_warn("[s] Listening on port %d", port);
  err = ne_net_listen(port, &server);
  if (err) return nerr_pass(err);

  err = ne_net_accept(&nsock, server, 10);
  if (err) return nerr_pass(err);

  ne_warn("[s] Connection.");

  do {
    err = ne_net_read_int(nsock, &x);
    if (err) break;

    if (x != uListLength(stuff))
    {
      err = nerr_raise(NERR_ASSERT, "Incoming length is not equal to expected length: %d != %d", x, uListLength(stuff));
      break;
    }

    for (x = 0; x < uListLength(stuff); x++)
    {
      err = uListGet(stuff, x, (void *)&thing);
      if (err) break;
      if (thing->is_num)
      {
	err = ne_net_read_int(nsock, &i);
	if (err) break;
	/* ne_warn("[s] Received %d", i); */
	if (thing->n != i)
	{
	  err = nerr_raise(NERR_ASSERT, "Incoming %d number is not equal to expected: %d != %d", x, i, thing->n);
	  break;
	}
      }
      else
      {
	err = ne_net_read_str_alloc(nsock, &s, NULL);
	if (err) break;
	/* ne_warn("[s] Received %s", s); */
	if (strcmp(s, thing->s))
	{
	  err = nerr_raise(NERR_ASSERT, "Incoming %d string is not equal to expected: '%s' != '%s'", x, s, thing->s);
	  break;
	}
	free(s);
      }
      printf("\rs");
    }
  } while (0);
  ne_net_close(&nsock);

  return nerr_pass(err);
}

NEOERR *run_test(void)
{
  NEOERR *err;
  ULIST *stuff;
  char word[64000];
  RAND_THING *thing;
  pid_t child;
  int x;

  ne_warn("starting net_test");
  ne_warn("generating random list");

  err = uListInit(&stuff, COUNT, 0);
  if (err) return nerr_pass(err);
  for (x = 0; x < COUNT; x++)
  {
    thing = (RAND_THING *) calloc(1, sizeof(RAND_THING));
    if (neo_rand(100) > 50)
    {
      thing->is_num = 1;
      thing->n = neo_rand(1000000);
    }
    else
    {
      neo_rand_word(word, sizeof(word));
      thing->s = strdup(word);
    }
    err = uListAppend(stuff, thing);
    if (err) return nerr_pass(err);
  }

  child = fork();
  if (!child)
  {
    /* child */
    return nerr_pass(client_proc(TEST_PORT, stuff));
  }
  /* parent */
  err = server_proc(TEST_PORT, stuff);

  if (!err) waitpid(child, NULL, 0);
  return nerr_pass(err);
}

int main(int argc, char **argv)
{
  NEOERR *err;

  nerr_init();
  err = run_test();
  if (err) 
  {
      nerr_log_error(err);
      return -1;
  }
  return 0;
}
