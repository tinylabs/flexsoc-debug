/**
 *  flexdbg entry point - Parse args and pass to daemon
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <argp.h>
#include <string.h>
#include <stdlib.h>

#include "flexdbg.h"
#include "log.h"

// Argument storage
static args_t args;

static int parse_opts (int key, char *arg, struct argp_state *state)
{
  switch (key) {
    // Append to load list
    case 'l':
      if (arg) {
        char *addr;
        args.load = (load_t *)realloc (args.load, sizeof (load_t) * (args.load_cnt + 1));

        // Parse into name/addr
        addr = strchr (arg, '@');
        if (addr) {
          *addr = '\0';
          addr++;
          args.load[args.load_cnt].addr = strtoul (addr, NULL, 0);
        }
        else
          args.load[args.load_cnt].addr = 0;
        
        args.load[args.load_cnt].name = (char *)malloc (strlen (arg) + 1);
        if (!args.load[args.load_cnt].name)
          log (LOG_FATAL, "Malloc failed!");
        strcpy (args.load[args.load_cnt].name, arg);
        args.load_cnt++;
      }
      break;

    case 'v':
      if (arg)
        args.verbose = strtoul (arg, NULL, 0);
      break;
      
    case ARGP_KEY_ARG:
      args.device = arg;
      break;
      
    // Check arguments
    case ARGP_KEY_END:
      if (!args.device)
        argp_usage (state);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

// Command line options
static struct argp_option options[] = {
                                       {0, 0, 0, 0, "Operations:", 1},
                                       {"load",    'l', "FILE", 0, "filename[@address] (default=0)\nmultiple load opts supported"},
                                       {"verbose", 'v', "INT", 0,  "verbosity level (0-4)"},
                                       {0}
};

struct argp flexdbg_argp = {options, &parse_opts, 0, 0};

int main (int argc, char **argv)
{
  int rv, i;
  
  // Clear args
  memset (&args, 0, sizeof (args));
  args.verbose = LOG_NORMAL;
        
  // Parse args
  argp_parse (&flexdbg_argp, argc, argv, 0, 0, 0);

  // Init logging
  log_init (args.verbose);
  
  // Do work
  rv = flexdbg (&args);

  // Clean up
  for (i = 0; i < args.load_cnt; i++)
    free (args.load[i].name);
  free (args.load);
  return rv;
}

