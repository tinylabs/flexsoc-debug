/*
 * Test triggering remote IRQ
 */

#include "Target.h"
#include "irq.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <cassert>

static uint8_t irq_idx = 0;
static uint8_t recvd_irq[10];
static Target *targ;
void irq_handler (uint8_t irq)
{
  printf ("IRQ => %u\n", irq);
  recvd_irq[irq_idx++] = irq;

  // Re-enable IRQ
  targ->WriteReg (0xE000E100, (1 << (irq-16)));
}

int main (int argc, char **argv)
{
  uint32_t val, i;

  // Connect to target
  Target *target = Target::Ptr (argv[1]);
  assert (target != NULL);

  // Save global pointer
  targ = target;
  
  // Validate CRC of CSR
  assert (target->Validate () == 0);

  // Set mode to JTAG
  target->SetPhy (PHY_SWD);

  // Send reset
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

  // Enable bridge
  target->BridgeEn (true);

  // Clear received IRQs
  memset (recvd_irq, 0, sizeof (recvd_irq));

  // Register IRQ handler
  target->RegisterIRQHandler (&irq_handler);

  // Enable IRQ scanning
  target->IRQScanEn (true);

  // Enable lower 4 IRQs
  target->WriteReg (0xE000E100, 0xF);
  
  // Init IRQs
  irq_init (8888);

  // Send IRQ 0
  pulse_irq (0);

  // Wait for response
  sleep (2);
  if ((irq_idx != 1) || (recvd_irq[0] != 16))
    assert (0);
  
  // Send IRQ 1
  pulse_irq (1);

  // Wait for response
  sleep (2);
  if ((irq_idx != 2) || (recvd_irq[1] != 17))
    assert (0);

  // Send IRQ 2
  pulse_irq (2);

  // Wait for response
  sleep (2);
  if ((irq_idx != 3) || (recvd_irq[2] != 18))
    assert (0);

  // Send IRQ 3
  pulse_irq (3);

  // Wait for response
  sleep (2);
  if ((irq_idx != 4) || (recvd_irq[3] != 19))
    assert (0);

  // Clean up IRQ
  irq_exit ();

  // Clean up target
  delete target;
  return 0;
}
