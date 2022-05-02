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
  uint32_t val;
  
  // Setup CSR - byte, no autoinc
  assert (target->WriteAP (0, 0xA2000000) == ADIv5_OK);

  // Write 4 bytes using DRW
  assert (target->WriteAP (4, 0x20000000) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x11) == ADIv5_OK);
  assert (target->WriteAP (4, 0x20000001) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x2200) == ADIv5_OK);
  assert (target->WriteAP (4, 0x20000002) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x330000) == ADIv5_OK);
  assert (target->WriteAP (4, 0x20000003) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x44000000) == ADIv5_OK);

  // Read back 4 bytes
  assert (target->WriteAP (4, 0x20000000) == ADIv5_OK);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert ((val & 0xFF) == 0x11);
  assert (target->WriteAP (4, 0x20000001) == ADIv5_OK);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert (((val>>8) & 0xFF) == 0x22);
  assert (target->WriteAP (4, 0x20000002) == ADIv5_OK);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert (((val>>16) & 0xFF) == 0x33);
  assert (target->WriteAP (4, 0x20000003) == ADIv5_OK);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);  
  assert (((val>>24) & 0xFF) == 0x44);
}

void mem_byte_autoinc (Target *target)
{
  uint32_t val;
  
  // Setup CSR - byte, autoinc
  assert (target->WriteAP (0, 0xA2000010) == ADIv5_OK);

  // Write 4 bytes using DRW
  assert (target->WriteAP (4, 0x20000000) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x11) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x2200) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x330000) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x44000000) == ADIv5_OK);

  // Reset TAR
  assert (target->WriteAP (4, 0x20000000) == ADIv5_OK);

  // Read back 4 bytes
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert ((val & 0xFF) == 0x11);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert (((val>>8) & 0xFF) == 0x22);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert (((val>>16) & 0xFF) == 0x33);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);  
  assert (((val>>24) & 0xFF) == 0x44);
}

void mem_hwrd_rw (Target *target)
{
  uint32_t val;
  
  // Setup CSR - hwrd, no autoinc
  assert (target->WriteAP (0, 0xA2000001) == ADIv5_OK);

  // Write 4 hwrd using DRW
  assert (target->WriteAP (4, 0x20000000) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x1111) == ADIv5_OK);
  assert (target->WriteAP (4, 0x20000002) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x22220000) == ADIv5_OK);
  assert (target->WriteAP (4, 0x20000004) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x3333) == ADIv5_OK);
  assert (target->WriteAP (4, 0x20000006) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x44440000) == ADIv5_OK);

  // Read back 4 hwrd
  assert (target->WriteAP (4, 0x20000000) == ADIv5_OK);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert ((val & 0xFFFF) == 0x1111);
  assert (target->WriteAP (4, 0x20000002) == ADIv5_OK);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert (((val>>16) & 0xFFFF) == 0x2222);
  assert (target->WriteAP (4, 0x20000004) == ADIv5_OK);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert ((val & 0xFFFF) == 0x3333);
  assert (target->WriteAP (4, 0x20000006) == ADIv5_OK);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert (((val>>16) & 0xFFFF) == 0x4444);
}

void mem_hwrd_autoinc (Target *target)
{
  uint32_t val;
  
  // Setup CSR - hwrd, autoinc
  assert (target->WriteAP (0, 0xA2000011) == ADIv5_OK);

  // Write 4 hwrd using DRW autoinc
  assert (target->WriteAP (4, 0x20000000) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x1111) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x22220000) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x3333) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x44440000) == ADIv5_OK);

  // Reset TAR
  assert (target->WriteAP (4, 0x20000000) == ADIv5_OK);

  // Read back 4 hwrd
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert ((val & 0xFFFF) == 0x1111);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert (((val>>16) & 0xFFFF) == 0x2222);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert ((val & 0xFFFF) == 0x3333);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);  
  assert (((val>>16) & 0xFFFF) == 0x4444);
}
  
void mem_word_rw (Target *target)
{
  uint32_t val;

  // Setup CSR - word, no autoinc
  assert (target->WriteAP (0, 0xA2000002) == ADIv5_OK);

  // Set TAR to beginning of memory
  assert (target->WriteAP (4, 0x20000000) == ADIv5_OK);

  // Write 4 words using BDn
  assert (target->WriteAP (0x10, 0x11223344) == ADIv5_OK);
  assert (target->WriteAP (0x14, 0x55667788) == ADIv5_OK);
  assert (target->WriteAP (0x18, 0x9900AABB) == ADIv5_OK);
  assert (target->WriteAP (0x1C, 0xCCDDEEFF) == ADIv5_OK);

  // Read back words
  assert (target->ReadAP (0x10, &val) == ADIv5_OK);
  assert (val == 0x11223344);
  assert (target->ReadAP (0x14, &val) == ADIv5_OK);
  assert (val == 0x55667788);
  assert (target->ReadAP (0x18, &val) == ADIv5_OK);
  assert (val == 0x9900AABB);
  assert (target->ReadAP (0x1C, &val) == ADIv5_OK);
  assert (val == 0xCCDDEEFF);
}

void mem_word_autoinc (Target *target)
{
  uint32_t val;

  // Setup CSR - word, autoinc
  assert (target->WriteAP (0, 0xA3000012) == ADIv5_OK);

  // Set TAR to beginning of memory
  assert (target->WriteAP (4, 0x20000000) == ADIv5_OK);

  // Write 4 words using DRW, autoinc
  assert (target->WriteAP (0xC, 0x11223344) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x55667788) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0x9900AABB) == ADIv5_OK);
  assert (target->WriteAP (0xC, 0xCCDDEEFF) == ADIv5_OK);

  // Reset TAR
  assert (target->WriteAP (4, 0x20000000) == ADIv5_OK);

  // Read back words using DRW, autoinc
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert (val == 0x11223344);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert (val == 0x55667788);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert (val == 0x9900AABB);
  assert (target->ReadAP (0xC, &val) == ADIv5_OK);
  assert (val == 0xCCDDEEFF);
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
  assert (target->WriteAP (0, 0xA2000002) == ADIv5_OK);

  // Set TAR to access DHCSR
  assert (target->WriteAP (4, 0xE000EDF0) == ADIv5_OK);

  // Try to halt processor
  for (i = 0; i < 10; i++) {

    // Write key + C_HALT to DHCSR
    assert (target->WriteAP (0xC, 0xA05F0003) == ADIv5_OK);

    // Check S_HALT to see if we halted
    assert (target->ReadAP (0xC, &val) == ADIv5_OK);
    if (val & (1 << 17))
      break;
  }

  // Check if halt failed
  assert (i != 10);
  
  // Check if byte/hwrd supported
  assert (target->WriteAP (0, 0xA3000000) == ADIv5_OK);
  assert (target->ReadAP (0, &val) == ADIv5_OK);
  assert ((val & 7) == 0);

  // Run byte tests
  mem_byte_rw (target);
  mem_byte_autoinc (target);

  // Run hword tests
  mem_hwrd_rw (target);
  mem_hwrd_autoinc (target);
  
  // Run word tests
  mem_word_rw (target);
  mem_word_autoinc (target);
  
  // Close device
  delete target;
  
  // Success
  return 0;
}
