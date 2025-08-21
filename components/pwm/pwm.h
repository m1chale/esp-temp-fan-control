#ifndef PWM_H
#define PWM_H
#include <stdint.h>

void init_pwm(uint8_t pwm_gpio);
void set_pwm_duty(uint8_t duty);

#endif