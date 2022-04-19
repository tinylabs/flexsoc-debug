
#include <stdint.h>
  
__attribute__((section (".vector1")))
uint32_t irq[240];


void isr (void)
{
  // Break to trigger debugger
  __asm ("bkpt #1");
}

int main (int  argc, char **argv)
{
  uint8_t i;
  
  // Point all IRQs to isr
  for (i = 0; i < 240; i++)
    irq[i] = (uint32_t)&isr;

  // Enable interrupts
  __asm ("cpsie i");
  
  // Busy loop
  while (1)
    ;
  return 0;
}

