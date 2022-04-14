/**
 *  flexsoc-debug test
 *
 *  Tiny Labs Inc
 *  2022
 */

#include <cassert>
#include "Target.h"


int main (int argc, char **argv)
{
  uint32_t devid;
  
  // Connect to target
  Target *target = Target::Ptr (argv[1]);
  assert (target != NULL);

  // Validate CRC of CSR
  assert (target->Validate () == 0);

  // Read device ID
  devid = target->FlexsocID ();
  assert ((devid >> 4) == 0xF1ecdb6);

  // Close device
  delete target;
  
  // Success
  return 0;
}
