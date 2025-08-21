#ifndef PWM_H
#define PWM_H
#include <stdint.h>

void init_pwm(void);
void set_pwm_duty(uint8_t duty);

#endif