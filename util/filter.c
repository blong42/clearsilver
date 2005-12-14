/* based on concepts from the mutt filter code... 
 *
 * This code basically does what popen should have been... and what
 * popen2/popen3/popen4 in python do... it allows you access to
 * as many of stdin/stdout/stderr for a sub program as you want, instead
 * of just one (which is what popen is).
 */

#include "cs_config.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/filter.h"


NEOERR *filter_wait (pid_t pid, int options, int *exitcode)
{
  int r;
  pid_t rpid;
  
  rpid = waitpid (pid, &r, options);
  if (WIFEXITED(r))
  {
    r = WEXITSTATUS(r);
    if (exitcode)
    {
      *exitcode = r;
      /* If they're asking for the exit code, we don't generate an error */
      return STATUS_OK;
    }
    if (r == 0) return STATUS_OK;
    else return nerr_raise(NERR_SYSTEM, "Child %d returned status %d:", rpid, 
                           r);
  }
  if (WIFSIGNALED(r))
  {
    r = WTERMSIG(r);
    return nerr_raise(NERR_SYSTEM, "Child %d died on signal %d:", rpid, r);
  }
  if (WIFSTOPPED(r))
  {
    r = WSTOPSIG(r);
    return nerr_raise(NERR_SYSTEM, "Child %d stopped on signal %d:", rpid, r);
  }
  
  return nerr_raise(NERR_ASSERT, "ERROR: waitpid(%d, %d) returned (%d, %d)", 
                    pid, options, rpid, r);
}

NEOERR *filter_create_fd (const char *cmd, int *fdin, int *fdout, int *fderr, 
                          pid_t *pid)
{
  int pi[2]={-1,-1}, po[2]={-1,-1}, pe[2]={-1,-1};
  int rpid;

  *pid = 0;

  if (fdin)
  {
    *fdin = 0;
    if (pipe (pi) == -1)
      return nerr_raise_errno(NERR_SYSTEM, 
                              "Unable to open in pipe for command: %s", cmd);
  }

  if (fdout)
  {
    *fdout = 0;
    if (pipe (po) == -1)
    {
      if (fdin)
      {
	close (pi[0]);
	close (pi[1]);
      }
      return nerr_raise_errno(NERR_SYSTEM, 
                              "Unable to open out pipe for command: %s", cmd);
    }
  }

  if (fderr)
  {
    *fderr = 0;
    if (pipe (pe) == -1)
    {
      if (fdin)
      {
	close (pi[0]);
	close (pi[1]);
      }
      if (fdout)
      {
	close (po[0]);
	close (po[1]);
      }
      return nerr_raise_errno(NERR_SYSTEM, "Unable to open err pipe for command: %s", cmd);
    }
  }

  /* block signals */

  if ((rpid = fork ()) == 0)
  {
    /* unblock signals */

    if (fdin)
    {
      close (pi[1]);
      dup2 (pi[0], 0);
      close (pi[0]);
    }

    if (fdout)
    {
      close (po[0]);
      dup2 (po[1], 1);
      close (po[1]);
    }

    if (fderr)
    {
      close (pe[0]);
      dup2 (pe[1], 2);
      close (pe[1]);
    }

    execl ("/bin/sh", "sh", "-c", cmd, (void *)NULL);
    _exit (127);
  }
  else if (rpid == -1)
  {
    /* unblock signals */
    if (fdin)
    {
      close (pi[0]);
      close (pi[1]);
    }
    if (fdout)
    {
      close (po[0]);
      close (po[1]);
    }
    if (fderr)
    {
      close (pe[0]);
      close (pe[1]);
    }
    return nerr_raise_errno(NERR_SYSTEM, "Unable to fork for command: %s", cmd);
  }

  if (fdout)
  {
    close (po[1]);
    *fdout = po[0];
  }
  if (fdin)
  {
    close (pi[0]);
    *fdin = pi[1];
  }
  if (fderr)
  {
    close (pe[1]);
    *fderr = pe[0];
  }
  *pid = rpid;

  return STATUS_OK;
}

NEOERR *filter_create_fp(const char *cmd, FILE **in, FILE **out, FILE **err, 
                         pid_t *pid)
{
  NEOERR *nerr;
  int fdin = 0, fdout = 0, fderr = 0;
  int *pfdin = NULL, *pfdout = NULL, *pfderr = NULL;

  if (in) pfdin = &fdin;
  if (out) pfdout = &fdout;
  if (err) pfderr = &fderr;

  nerr = filter_create_fd(cmd, pfdin, pfdout, pfderr, pid);
  if (nerr) return nerr_pass(nerr);

  if (in)
  {
    *in = fdopen (fdin, "w");
    if (*in == NULL)
      return nerr_raise_errno(NERR_IO, "Unable to fdopen in for command: %s", 
	  cmd);
  }

  if (out)
  {
    *out = fdopen (fdout, "r");
    if (*out == NULL)
    {
      if (in) fclose(*in);
      return nerr_raise_errno(NERR_IO, "Unable to fdopen out for command: %s", 
	  cmd);
    }
  }

  if (err)
  {
    *err = fdopen (fderr, "r");
    if (*err == NULL)
    {
      if (in) fclose(*in);
      if (out) fclose(*out);
      return nerr_raise_errno(NERR_IO, "Unable to fdopen err for command: %s", 
	  cmd);
    }
  }
  return STATUS_OK;
}
