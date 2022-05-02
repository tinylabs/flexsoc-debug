/*
 * Test triggering remote IRQ
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "irq.h"

int main (int argc, char **argv)
{
  if (argc != 2) {
    printf ("invalid args!\n");
    return -1;
  }
    
  // Init IRQs
  irq_init (8888);

  // Send IRQ 0
  pulse_irq (atoi (argv[1]));

  // Clean up IRQ
  irq_exit ();

  return 0;
}
