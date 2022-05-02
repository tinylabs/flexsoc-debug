/*
 * Test triggering remote IRQ
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <cassert>

#include "Target.h"
#include "Debug.h"
#include "irq.h"

static uint8_t irq_idx = 0;
static uint8_t recvd_irq[10];
static Target *targ;

void irq_handler (uint8_t ctl, uint8_t irq)
{
  printf ("IRQ => %u\n", irq);
  recvd_irq[irq_idx++] = irq;

  // Acknowledge IRQ
  targ->IRQAck (ctl);
}

int main (int argc, char **argv)
{
  uint32_t irq_buf, val, i;
  Debug *debug;
  
  // Connect to target
  Target *target = Target::Ptr (argv[1]);
  assert (target != NULL);

  // Save global pointer
  targ = target;

  // Set mode to JTAG
  target->SetPhy (PHY_SWD);

  // Send reset
  target->Reset (1);

  // Enable AP access
  target->EnableAP (true);

  // Enable bridge
  target->BridgeEn (true);

  // Create debug interface
  debug = new Debug (target);

  // Reset and halt target
  assert (debug->Halt (true) == SUCCESS);

  // Load exe to RAM
  assert (debug->LoadBin (0x20000000, argv[2]) == SUCCESS);

  // Set PC to exe
  assert (debug->RegWrite (REG_PC, 0x20000001) == SUCCESS);
  
  // Clear received IRQs
  memset (recvd_irq, 0, sizeof (recvd_irq));

  // Register IRQ handler
  target->RegisterIRQHandler (&irq_handler);

  // Get IRQ base from image offset +4
  irq_buf = target->ReadReg (0x20000004);

  // Set IRQ buf for hardware
  target->BridgeIRQBuf (irq_buf);
  
  // Run target
  assert (debug->Run () == SUCCESS);
  
  // Enable IRQ scanning
  target->BridgeIRQScanEn (true);

  // Enable lower 4 IRQs
  target->WriteReg (0xE000E100, 0xF);
  
  // Init IRQs
  irq_init (8888);

  // Delay to allow simulated hardware to catch up
  sleep (5);
  
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
