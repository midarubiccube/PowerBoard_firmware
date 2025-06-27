#include "tim.h"

class FullColorLED
{
private:
    uint32_t TIM_CHANNEL_X;
    TIM_HandleTypeDef* HTIM;
    #define HIGH 13 //ビット値1
    #define LOW 7 //ビット値0
    uint8_t rgb_buf[3]; //色データの配列。1つのNeoPixelにつきGRBの3色分のデータがあります。
    uint32_t pwm_buf[324]={0}; //PWM配列
public:
    FullColorLED(TIM_HandleTypeDef* htim, uint32_t tim_channel_x);
    void stopLED();
    void set_rgb(uint8_t r, uint8_t g, uint8_t b);
    void start();
};