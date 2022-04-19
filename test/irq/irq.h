#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

void irq_init (uint16_t port);
void irq_exit (void);
void send_irq (uint8_t irq, uint8_t active);
void pulse_irq (uint8_t irq);

#endif /* IRQ_H */
