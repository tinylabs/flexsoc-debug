/**
 *  flexsoc-debug test
 *
 *  Tiny Labs Inc
 *  2022
 */

#include <cassert>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "Target.h"
#include "log.h"

// Global buffers
static uint32_t data[256];
static uint32_t verify[256];

void test_word (Target *target, uint32_t *data, size_t len)
{
  // Clear verify data
  memset (verify, 0, sizeof (verify));
  
  // Enable bridge
  target->BridgeEn (true);

  // Setup bridge mode
  target->BridgeMode (MODE_NORMAL);
  
  // Write data
  target->WriteW (0x20000000, data, len);

  // Read back
  target->ReadW (0x20000000, verify, len);

  // Verify integrity
  if (memcmp (data, verify, sizeof (data)))
    assert (0);

  // Disable bridge
  target->BridgeEn (false);  
}

void test_word_autoinc (Target *target, uint32_t *data, size_t len)
{
  // Clear verify data
  memset (verify, 0, sizeof (verify));
  
  // Enable bridge
  target->BridgeEn (true);

  // Setup bridge mode
  target->BridgeMode (MODE_SEQUENTIAL);
  
  // Write data
  target->WriteW (0x20000000, data, len);

  // Read back
  target->ReadW (0x20000000, verify, len);

  // Verify integrity
  if (memcmp (data, verify, sizeof (data)))
    assert (0);

  // Disable bridge
  target->BridgeEn (false);  
}

void test_hwrd (Target *target, uint16_t *data, size_t len)
{
    // Clear verify data
  memset (verify, 0, sizeof (verify));
  
  // Enable bridge
  target->BridgeEn (true);

  // Setup bridge mode
  target->BridgeMode (MODE_NORMAL);
  
  // Write data
  target->WriteH (0x20000000, data, len);

  // Read back
  target->ReadH (0x20000000, (uint16_t *)verify, len);

  // Verify integrity
  if (memcmp (data, (uint16_t *)verify, sizeof (data)))
    assert (0);

  // Disable bridge
  target->BridgeEn (false);  
}

void test_hwrd_autoinc (Target *target, uint16_t *data, size_t len)
{
    // Clear verify data
  memset (verify, 0, sizeof (verify));
  
  // Enable bridge
  target->BridgeEn (true);

  // Setup bridge mode
  target->BridgeMode (MODE_SEQUENTIAL);
  
  // Write data
  target->WriteH (0x20000000, data, len);

  // Read back
  target->ReadH (0x20000000, (uint16_t *)verify, len);

  // Verify integrity
  if (memcmp (data, (uint16_t *)verify, sizeof (data)))
    assert (0);

  // Disable bridge
  target->BridgeEn (false);  
}

void test_byte (Target *target, uint8_t *data, size_t len)
{
    // Clear verify data
  memset (verify, 0, sizeof (verify));
  
  // Enable bridge
  target->BridgeEn (true);

  // Setup bridge mode
  target->BridgeMode (MODE_NORMAL);
  
  // Write data
  target->WriteB (0x20000000, data, len);

  // Read back
  target->ReadB (0x20000000, (uint8_t *)verify, len);

  // Verify integrity
  if (memcmp (data, (uint8_t *)verify, sizeof (data)))
    assert (0);

  // Disable bridge
  target->BridgeEn (false);
}

void test_byte_autoinc (Target *target, uint8_t *data, size_t len)
{
    // Clear verify data
  memset (verify, 0, sizeof (verify));
  
  // Enable bridge
  target->BridgeEn (true);

  // Setup bridge mode
  target->BridgeMode (MODE_SEQUENTIAL);
  
  // Write data
  target->WriteB (0x20000000, data, len);

  // Read back
  target->ReadB (0x20000000, (uint8_t *)verify, len);

  // Verify integrity
  if (memcmp (data, (uint8_t *)verify, sizeof (data)))
    assert (0);

  // Disable bridge
  target->BridgeEn (false);  
}

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

  // Always use AP0 = MEM-AP
  target->BridgeAPSel (0);

  int count = sizeof (data) / sizeof (uint32_t);
  
  // Generate 1k random data
  srand (time (NULL));
  for (i = 0; i < count; i++)
    data[i] = rand ();

  // Test word
  test_word (target, data, count);
  test_word_autoinc (target, data, count);
  
  // Test hwrd
  test_hwrd (target, (uint16_t *)data, count * 2);
  test_hwrd_autoinc (target, (uint16_t *)data, count * 2);
  
  // Test Byte
  test_byte (target, (uint8_t *)data, count * 4);
  test_byte_autoinc (target, (uint8_t *)data, count * 4);
    
  // Close device
  delete target;
  
  // Success
  return 0;
}
