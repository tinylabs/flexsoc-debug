/**
 *  flexsoc-debug test
 *
 *  Tiny Labs Inc
 *  2022
 */

#include <cassert>
#include <unistd.h>
#include "Target.h"
#include "log.h"

int main (int argc, char **argv)
{
  uint32_t i, val = 0;

  // Connect to target
  Target *target = Target::Ptr (argv[1]);
  assert (target != NULL);

  // Validate CRC of CSR
  assert (target->Validate () == 0);

  // Set phy to SWD
  target->SetPhy (PHY_SWD);

  // Send reset + protocol switch
  target->Reset (1);

  // Enable debug for AP access
  assert (target->WriteDP (4, 0x50000000) == ADIv5_OK);

  // Poll for ACK
  for (i = 0; i < 10; i++) {
    assert (target->ReadDP (4, &val) == ADIv5_OK);
    if ((val & 0xF0000000) == 0xF0000000)
      break;
  }

  // Check for ACK
  assert ((val & 0xF0000000) == 0xF0000000);

  // Now we can access MEM-AP
  // Read MEMAP-IDR
  assert (target->ReadAP (0, 0xfc, &val) == ADIv5_OK);
  assert (val == 0x24770011);

  // Close device
  delete target;
  
  // Success
  return 0;
}
