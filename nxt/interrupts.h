#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

int interrupts_get(void);
int interrupts_get_and_disable(void);
void interrupts_enable(void);

int fiq_get_and_disable(void);
void fiq_enable(void);

#endif
