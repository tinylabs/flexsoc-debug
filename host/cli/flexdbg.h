/**
 *  flexdbg header file - Args to call flexdbg
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#ifndef FLEXDBG_H
#define FLEXDBG_H

#include <stdint.h>

typedef struct {
  char    *name;
  uint32_t addr;
} load_t;

typedef struct {
  char    *device;     // Device to connect to
  load_t  *load;       // List of files to load
  int     load_cnt;    // Number of files to load
  int     verbose;     // 0=off 3=max
} args_t;

int flexdbg (args_t *args);

#endif /* FLEXDBG_H */
