/**
 *  Helper interface to debug functions of target
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2022
 */
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>

#include "Target.h"
#include "Debug.h"

// Register definitions
// Debug Halting Control and Status Register
#define DHCSR       0xE000EDF0
#define C_KEY       0xA05F0000
#define C_DEBUGEN   (1 << 0)
#define C_HALT      (1 << 1)
#define C_STEP      (1 << 2)
#define C_MASKINTS  (1 << 3)
#define S_REGRDY    (1 << 16)
#define S_HALT      (1 << 17)
#define S_SLEEP     (1 << 18)
#define S_LOCKUP    (1 << 19)
#define S_RESET     (1 << 25)

// Debug Core Register Selector
#define DCRSR       0xE000EDF4
#define REG_WnR     (1 << 16)

// Debug Core Register Data
#define DCRDR         0xE000EDF8

// Debug Exception and Monitor Control Register
#define DEMCR         0xE000EDFC
#define VC_CORERESET  (1 << 0)
#define VC_MMERR      (1 << 3)
#define VC_NOCPERR    (1 << 5)
#define VC_CHKERR     (1 << 6)
#define VC_STATERR    (1 << 7)
#define VC_BUSERR     (1 << 8)
#define VC_INTERR     (1 << 9)
#define VC_HARDERR    (1 << 10)
#define MON_EN        (1 << 16)
#define MON_PEND      (1 << 17)
#define MON_STEP      (1 << 18)
#define MON_REQ       (1 << 19)
#define TRACENA       (1 << 24)

// Application Interrupt and Reset Control Register
#define AIRCR         0xE000ED0C
#define SYSRESETREQ   (1 << 2)

// Debug Fault Status Register
#define DFSR          0xE000ED30
#define EVT_HALTED    (1 << 0)
#define EVT_BKPT      (1 << 1)
#define EVT_DWTTRAP   (1 << 2)
#define EVT_VCATCH    (1 << 3)
#define EVT_EXTERNAL  (1 << 4)
#define EVT_CLRMASK   0x1F

Debug::Debug (Target *t)
{
  // Save target
  this->target = t;
}

Debug::~Debug ()
{
  
}

int Debug::Halt (bool do_reset)
{
  int i;

  // Do reset then halt
  if (do_reset) {
    
    // Setup to halt on reset
    target->WriteReg (DHCSR, C_KEY | C_HALT | C_DEBUGEN);
    target->WriteReg (DEMCR, VC_CORERESET);

    // Initiate full system reset (minus debug)
    target->WriteReg (AIRCR, SYSRESETREQ);

    // Sleept for reset to complete
    sleep (1);
    
    // Make sure halted
    if (target->ReadReg ((DHCSR & S_HALT) == 0))
      return -ERR_NOHALT;
  }
  else {

    // Try to halt running processor
    for (i = 0; i < timeout; i++) {
    
      // Write key + C_HALT to DHCSR
      target->WriteReg (DHCSR, C_KEY | C_HALT | C_DEBUGEN);
      
      // Check S_HALT to see if we halted
      if (target->ReadReg (DHCSR) & S_HALT)
        break;
    }

    // Check if we timed out
    if (i == timeout)
      return -ERR_NOHALT;
  }

  // Clear DFSR events
  target->WriteReg (DFSR, EVT_CLRMASK);
  
  // Success
  return SUCCESS;
}

int Debug::Run (void)
{
  // Clear all DFSR
  target->WriteReg (DFSR, EVT_CLRMASK);

  // Clear halt and unset debug enable
  target->WriteReg (DHCSR, C_KEY);
  return SUCCESS;
}

int Debug::Step (void)
{
  int rv, i;
  
  // Make sure processor is halted
  if ((target->ReadReg (DHCSR) & S_HALT) != 0) {

    // Try and halt
    rv = Halt (false);
    if (rv)
      return rv;
  }

  // Set C_STEP and clear C_HALT
  target->WriteReg (DHCSR, C_KEY | C_STEP | C_DEBUGEN);

  // Wait for S_HALT to indicate step complete
  for (i = 0; i < timeout; i++)
    if (target->ReadReg (DHCSR) & S_HALT)
      break;

  // Check if we succeeded
  if (i == timeout)
    return -ERR_TIMEOUT;
  return SUCCESS;
}

int Debug::RegRead (reg_t reg, uint32_t *val)
{
  int i;

  if (!val)
    return -ERR_PARAMS;

  // Write register to read
  target->WriteReg (DCRSR, reg);

  // Poll for completion
  for (i = 0; i < timeout; i++)
    if (target->ReadReg (DHCSR) & S_REGRDY)
      break;

  // Check if we succeeded
  if (i == timeout)
    return -ERR_TIMEOUT;

  *val = target->ReadReg (DCRDR);
  return SUCCESS;
}

int Debug::RegWrite (reg_t reg, uint32_t val)
{
  int i;

  // Write data
  target->WriteReg (DCRDR, val);

  // Write command
  target->WriteReg (DCRSR, reg | REG_WnR);
  
  // Poll for completion
  for (i = 0; i < timeout; i++)
    if (target->ReadReg (DHCSR) & S_REGRDY)
      break;

  // Check if we succeeded
  if (i == timeout)
    return -ERR_TIMEOUT;
  return SUCCESS;
}

int Debug::LoadBin (uint32_t addr, const char *filename)
{
  size_t sz;
  uint8_t *bin;
  FILE *fbin;
  struct stat fstat;
  
  // Open file
  fbin = fopen (filename, "rb");
  if (!fbin)
    return -ERR_PARAMS;

  // Get file size
  if (stat (filename, &fstat) == -1)
    return -ERR_PARAMS;
  sz = fstat.st_size;

  // Round to nearest word
  sz = (sz % 4) != 0 ? sz + 4 - (sz % 4) : sz;
  
  // Malloc space for file
  bin = (uint8_t *)malloc (sz);
  if (!bin)
    return -ERR_NOMEM;
  
  // Read entire file
  fread (bin, fstat.st_size, 1, fbin);
  
  // Close file
  fclose (fbin);
  
  // Enable sequential access for max throughput
  target->BridgeMode (MODE_SEQUENTIAL);
  
  // Write to address space
  target->WriteW (addr, (uint32_t *)bin, sz/4);

  // Free memory
  free (bin);
  return 0;
}

