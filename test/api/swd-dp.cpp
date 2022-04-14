/**
 *  flexsoc-debug test
 *
 *  Tiny Labs Inc
 *  2022
 */

#include <cassert>
#include "Target.h"
#include "log.h"

int main (int argc, char **argv)
{
  uint32_t val;
  
  // Connect to target
  Target *target = Target::Ptr (argv[1]);
  assert (target != NULL);

  // Validate CRC of CSR
  assert (target->Validate () == 0);

  // Set mode to JTAG
  target->Mode (MODE_SWD);

  // Send reset + protocol switch
  target->Reset (1);

  // Read DPIDR
  assert (target->ReadDP (0, &val) == ADIv5_OK);
  assert (val == 0x2BA01477);

  // Close device
  delete target;
  
  // Success
  return 0;
}
