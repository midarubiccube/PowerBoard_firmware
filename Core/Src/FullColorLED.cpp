#include "FullColorLED.hpp"
#include "tim.h"

FullColorLED::FullColorLED(TIM_HandleTypeDef* htim, uint32_t tim_channel_x){
    HTIM = htim;
    TIM_CHANNEL_X = tim_channel_x;
}


void FullColorLED::stopLED(){
    HAL_TIM_PWM_Stop_DMA(HTIM, TIM_CHANNEL_X);
}

void FullColorLED::start(){
    HAL_TIM_PWM_Start_DMA(HTIM, TIM_CHANNEL_X, (uint32_t *)pwm_buf, 324);
}

void FullColorLED::set_rgb(uint8_t r, uint8_t g,uint8_t b){
    for(char i = 0; i < 8; ++i) {
        pwm_buf[i + 300] = ((g>>(7-i))&1) ? HIGH : LOW;
        pwm_buf[i + 308] = ((r>>(7-i))&1) ? HIGH : LOW;
        pwm_buf[i + 316] = ((b>>(7-i))&1) ? HIGH : LOW;
    }
}
