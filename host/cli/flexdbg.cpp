/**
 * flexsoc_cm3 main application
 *
 * All rights reserved.
 * Tiny Labs Inc
 * 2020
 */

#include "flexdbg.h"

#include "Target.h"
#include "flexsoc.h"
#include "remote.h"
#include "log.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>

static int shutdown_flag = 0;

// Handle shutdown - this could be caught from ^C or a system unit test completed
static void shutdown (int sig)
{
  signal (SIGINT, SIG_DFL);
  log (LOG_NORMAL, "Shutting down...");
  shutdown_flag = 1;  
}

static uint32_t *read_bin (const char *filename, long *size)
{
  FILE *fp;
  long nsize, bread = 0;
  uint32_t *buf;
  int rv, i;
  
  // Open file
  fp = fopen (filename, "rb");
  if (!fp)
    log (LOG_FATAL, "Failed to open: %s", filename);

  // Get file size
  fseek (fp, 0, SEEK_END);
  *size = ftell (fp);
  fseek (fp, 0, 0);
  if (*size & 3)
    nsize = *size + 4 - (*size & 3);
  else
    nsize = *size;
  
  // Malloc space
  buf = (uint32_t *)malloc (nsize);
  if (!buf)
    log (LOG_FATAL, "Failed to malloc");
  memset (buf, 0, nsize);
  
  // Read into buffer
  while (bread < *size) {
    rv = fread (&buf[bread], 1, *size - bread, fp);
    if (rv <= 0)
      log (LOG_FATAL, "IO error: %d", rv);
    bread += rv;
  }

  // Return nsize
  *size = nsize;
  
  // Close file and return
  fclose (fp);
  return buf;
}

int flexdbg (args_t *args)
{
  Target *target;
  uint32_t devid, val;
  char *device;
  adiv5_stat_t stat;
  
  // Copy device as it's modified inplace if simulator
  device = (char *)malloc (strlen (args->device + 1));
  strcpy (device, args->device);
  
  // Find hardware
  target = Target::Ptr (device);

  // Check interface
  if (target->Validate ())
    log (LOG_FATAL, "CSR mismatch - Regenerate FPGA/CSR!");
  
  // Read device_id
  devid = target->FlexsocID ();
  if ((devid >> 4) == 0xF1ecdb6)
    log_nonl (LOG_NORMAL, "FlexDebug v%d\n", devid & 0xf);
  else
    log (LOG_ERR, "FlexDebug not found!\n");

  // Register handler for shutdown
  signal (SIGINT, &shutdown);

  // Switch to SWD
  target->Mode (MODE_SWD);
  printf ("MODE SWD\n");

  // Send reset + protocol switch
  target->Reset (1);

  // Read IDcode
  stat = target->ReadDP (0, &val);
  if (stat == ADIv5_OK)
    printf ("IDCODE=%08X\n", val);
  else
    printf ("Error: %s\n", target->ADIv5_Stat (stat));

  // Enable debug
  stat = target->WriteDP (4, 0x50000000);
  printf ("%s\n", target->ADIv5_Stat (stat));
  stat = target->ReadDP (4, &val);
  printf ("%08X %s\n", val, target->ADIv5_Stat (stat));

  // Read AP[0] IDR
  stat = target->ReadAP (0, 0xfc, &val);
  printf ("%08X %s\n", val, target->ADIv5_Stat (stat));

  // Change div
  target->ClkDiv (2);
  
  // Switch to JTAG
  target->Mode (MODE_JTAG);
  printf ("MODE JTAG\n");

  // Send reset + switch
  target->Reset (1);

  // Read IDcode
  stat = target->ReadDP (0, &val);
  if (stat == ADIv5_OK)
    printf ("IDCODE=%08X\n", val);
  else
    printf ("Error: %s\n", target->ADIv5_Stat (stat));

  // Enable debug
  stat = target->WriteDP (4, 0x50000000);
  printf ("%s\n", target->ADIv5_Stat (stat));
  stat = target->ReadDP (4, &val);
  printf ("%08X %s\n", val, target->ADIv5_Stat (stat));

  // Read AP[0] IDR
  stat = target->ReadAP (0, 0xfc, &val);
  printf ("%08X %s\n", val, target->ADIv5_Stat (stat));

  // Close device
  delete target;

  // Success
  return flexsoc_read_returnval();
}
