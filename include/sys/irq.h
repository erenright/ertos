#ifndef _IRQ_H
#define _IRQ_H

// arch/irq.s
void sti(void);
void cli(void);
void enable_irq(int irq);
void disable_irq(int irq);

// kernel/irq.c
void c_irq(void);
int register_irq_handler(int irq, void *handler);

#endif // !_IRQ_H
