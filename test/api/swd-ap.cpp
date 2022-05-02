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
  assert (i != 10);

  // Now we can access MEM-AP
  // Read MEMAP-IDR
  assert (target->ReadAP (0xfc, &val) == ADIv5_OK);
  assert (val == 0x24770011);

  // Write test data for BD0 read
  assert (target->WriteAP (4, 0x20000000) == ADIv5_OK);
  assert (target->WriteAP (0, 0xA3000002) == ADIv5_OK);
  assert (target->WriteAP (0xc, 0x11223344) == ADIv5_OK);

  // Read from each AP bank
  assert (target->ReadAP (0xe0, &val) == ADIv5_OK);
  assert (val == 0);
  assert (target->ReadAP (0x30, &val) == ADIv5_OK);
  assert (val == 0);
  assert (target->ReadAP (0x10, &val) == ADIv5_OK);
  assert (val == 0x11223344);
  assert (target->ReadAP (0x00, &val) == ADIv5_OK);
  assert (val == 0x23000042);

  // Close device
  delete target;
  
  // Success
  return 0;
}
