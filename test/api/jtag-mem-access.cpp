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


void mem_byte_rw (Target *target)
{
  
}

void mem_byte_autoinc (Target *target)
{
}

void mem_hwrd_rw (Target *target)
{

}
void mem_hwrd_autoinc (Target *target)
{
}
  
void mem_word_rw (Target *target)
{
  
}

void mem_word_autoinc (Target *target)
{
}

int main (int argc, char **argv)
{
  uint32_t i, val = 0;

  // Connect to target
  Target *target = Target::Ptr (argv[1]);
  assert (target != NULL);

  // Validate CRC of CSR
  assert (target->Validate () == 0);

  // Set mode to JTAG
  target->SetPhy (PHY_JTAG);

  // Send reset + protocol switch
  target->Reset (0);

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

  // Write CSW for word access
  assert (target->WriteAP (0, 0, 0xA2000002) == ADIv5_OK);

  // Set TAR to access DHCSR
  assert (target->WriteAP (0, 4, 0xE000EDF0) == ADIv5_OK);

  // Try to halt processor
  for (i = 0; i < 10; i++) {

    // Write key + C_HALT to DHCSR
    assert (target->WriteAP (0, 0xC, 0xA05F0003) == ADIv5_OK);

    // Check S_HALT to see if we halted
    assert (target->ReadAP (0, 0xC, &val) == ADIv5_OK);
    if (val & (1 << 17))
      break;
  }

  // Check if halt failed
  assert (i != 10);

  // Check if byte/hwrd supported
  assert (target->WriteAP (0, 0, 0xA2000000) == ADIv5_OK);
  assert (target->ReadAP (0, 0, &val) == ADIv5_OK);
  assert ((val & 7) == 0);

  // Run byte tests
  mem_byte_rw (target);
  mem_byte_autoinc (target);

  // Run hword tests
  assert (target->WriteAP (0, 0, 0xA2000001) == ADIv5_OK);
  mem_hwrd_rw (target);
  mem_hwrd_autoinc (target);
  
  // Run word tests
  assert (target->WriteAP (0, 0, 0xA2000002) == ADIv5_OK);
  mem_word_rw (target);
  mem_word_autoinc (target);
  
  // Close device
  delete target;
  
  // Success
  return 0;
}
