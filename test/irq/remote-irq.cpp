/*
 * Test triggering remote IRQ
 */

#include "irq.h"
#include "Target.h"

int main (int argc, char **argv)
{
  // Init IRQs
  irq_init (8888);

  // Send IRQ 0
  pulse_irq (0);

  // Check if we received IRQ
  
  // Clean up IRQ
  irq_exit ();
}
