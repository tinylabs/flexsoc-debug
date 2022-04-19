#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void irq_init (uint16_t port);
void irq_exit (void);
void send_irq (uint8_t irq, uint8_t active);
void pulse_irq (uint8_t irq);
#ifdef __cplusplus
}
#endif
#endif /* IRQ_H */
