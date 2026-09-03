#include "main.hpp"

#include <cstring>

#include "main.h"
#include "cmsis_os2.h"
#include "stdio.h"

#include "spi.h"
#include "adc.h"

#include "CANFD.hpp"
#include "FullColorLED.hpp"
#include "stm32g4xx_hal_gpio.h"

CANFD* canfd;
FullColorLED led{&htim1, TIM_CHANNEL_1};
int data;

extern osTimerId_t dischargeTimerHandle;
extern osTimerId_t cantx_taskHandle;
extern DMA_HandleTypeDef hdma_adc1;


bool onoff = false;

#define MCP3208_CS_LOW()  HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)
#define MCP3208_CS_HIGH() HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)

#define MCP3208_START_BIT   0x04
#define MCP3208_MODE_SINGLE 0x02
#define MCP3208_MODE_DIFF   0x00

uint16_t ADC_buff[3];
ID  own_id;


uint16_t MCP3208_Read(uint8_t channel)
{
  uint8_t tx[3];
  uint8_t rx[3];
  MCP3208_CS_LOW();

  tx[0] = MCP3208_START_BIT | MCP3208_MODE_SINGLE | ((channel & 0x04) >> 2);
  tx[1] = (channel & 0x03) << 6;
  tx[2] = 0;

  HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3, HAL_MAX_DELAY);

  MCP3208_CS_HIGH();

  uint16_t value;
  value = ((rx[1] & 0x0F) << 8) | rx[2];

  return value;
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) {
	  canfd->rx_interrupt_task();
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == EMENGECY_Pin)
  {
    if (HAL_GPIO_ReadPin(EMENGECY_GPIO_Port, EMENGECY_Pin) == GPIO_PIN_RESET)
    {
      HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);
    }
    else
    {
      CANFD_Frame emengency_msg;
      /*Message_format msg = {0};
      msg.id.format.message_type = 0;
      msg.id.format.from_id = 0; 
      msg.id.format.from_type = 1;
      emengency_msg.id = msg.id.id;
      emengency_msg.is_remote = true;
      emengency_msg.size = 0;
      canfd->tx(emengency_msg);*/
      HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_SET);
      osTimerStart(dischargeTimerHandle, 300);
    }
  }
}


extern "C" void StartDefaultTask(void *argument)
{
  led.set_rgb(255, 0, 0);
  led.start();
  canfd = new CANFD(&hfdcan1);
	canfd->start();

  own_id.fields.board_num = 0;
  own_id.fields.data_type = DataType::POWERBOARD_COMANND;
  canfd->set_filter_mask(own_id.id, 0xff);
  
  osTimerStart(cantx_taskHandle, 10);
  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADC_buff, sizeof(ADC_buff) / sizeof(ADC_buff[0]));
  hdma_adc1.Instance->CCR &= ~(DMA_IT_TC | DMA_IT_HT);

  while (1)
  {
    if (canfd->rx_available())
    {
      CANFD_Frame receive;
      canfd->rx(receive);
      PWRPacket packet;
      memcpy(&packet, receive.data, receive.size);
      if (packet.pwrstatus == 1){
		    led.set_rgb(0, 255, 0);
        HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);
        osDelay(10);
        HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_SET);
        onoff = true;
	    } else {
		    led.set_rgb(255, 0, 0);
        HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_RESET);
        osDelay(30);
        HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_SET);
        osTimerStart(dischargeTimerHandle, 300);
        onoff = false;
	    }
    }
    osDelay(10);
  }
}

extern "C" void cantxCallback(void *argument)
{
  printf("%f %f %f\n", MCP3208_Read(2)/122.0, MCP3208_Read(1)/122.0, MCP3208_Read(0)/122.0);
  float current = (ADC_buff[0] - 410) / 62.0;
  if (current > 10.0f)
  {
    HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_RESET);
  }

  PWRXPacket packet;
  packet.current = current;
  packet.battery1_voltage = MCP3208_Read(2)/122.0;
  packet.battery2_voltage = MCP3208_Read(1)/122.0;
  packet.output_voltage = MCP3208_Read(0)/122.0;
  CANFD_Frame send;
  memcpy(send.data, &packet, sizeof(packet));
  send.id = own_id.id;
  send.size = sizeof(packet);
  canfd->tx(send);
}

extern "C" void dischargeCallback(void *argument)
{
    HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);
}