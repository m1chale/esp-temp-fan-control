#include "driver/ledc.h"
#include "esp_err.h"

#define PWM_FREQ_HZ 25000        // 25 kHz
#define PWM_RES LEDC_TIMER_8_BIT // resolution (0–255)
#define PWM_CHANNEL LEDC_CHANNEL_0

void init_pwm(uint8_t pwm_gpio)
{
    // Timer konfigurieren
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RES,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&timer_conf);

    // Kanal konfigurieren
    ledc_channel_config_t channel_conf = {
        .gpio_num = pwm_gpio,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = PWM_CHANNEL,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&channel_conf);
}

// 0 = 0%
// 128 = 50 %
// 255 = 100 %
void set_pwm_duty(uint8_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);
}