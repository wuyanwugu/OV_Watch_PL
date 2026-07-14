/* Private includes -----------------------------------------------------------*/

// includes
// sys
#include "usart.h"
#include "tim.h"
#include "stm32f4xx_it.h"
#include "delay.h"

// user
#include "user_TasksInit.h"
#include "HWDataAccess.h"
#include "version.h"

// bsp
#include "key.h"
#include "lcd.h"
#include "CST816.h"
#include "rtc.h"
#include "power.h"
#include "MPU6050.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "DataSave.h"
#include "AHT21.h"
#include "SPL06_001.h"
#include "WDOG.h"

// ui
#include "lvgl.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"
#include "ui.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern uint8_t HardInt_receive_str[25];
uint8_t ui_LTimeValue;   // 暗屏时间 (秒)
uint8_t ui_TTimeValue;   // 熄屏时间 (秒)
/* Private function prototypes -----------------------------------------------*/
void LED_Port_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin : PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
  * @brief  hardwares init task
  * @param  argument: Not used
  * @retval None
  */
void HardwareInitTask(void *argument)
{
	while(1)
	{
    vTaskSuspendAll();

     LED_Port_Init();
     
    // RTC Wake timer
    if(HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2000, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK)
    {
      Error_Handler();
    }

    // UART DMA - BLE 模块通信
    HAL_UART_Receive_DMA(&huart1, (uint8_t*)HardInt_receive_str, 25);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

    // BLE Init + Disable — PA8 拉低，防止浮空意外使能 BLE 模块耗电
    HWInterface.BLE.Init();
    HWInterface.BLE.Disable();

    // PWM Start - backlight
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

    // sys delay
    delay_init();

    // power
    HWInterface.Power.Init();

    // key
    Key_Port_Init();

    // IMU
    uint8_t num = 3;
    while(num && HWInterface.IMU.ConnectionError)
    {
      num--;
      HWInterface.IMU.ConnectionError = HWInterface.IMU.Init();
    }

    // EEPROM - 读取设置和步数
    EEPROM_Init();
    if(!EEPROM_Check())
    {
      // 读取 wrist_is_enabled 设置
      uint8_t wrist_setting[1];
      SettingGet(wrist_setting, 0x10, 1);
      if(wrist_setting[0] == 1)
      {
        HWInterface.IMU.wrist_is_enabled = 1;
      }
      else
      {
        HWInterface.IMU.wrist_is_enabled = 0;
      }

      // 读取熄屏时间设置 (EEPROM地址 0x12)
      // ui_TTimeValue = 总亮屏超时(秒), ui_LTimeValue = 暗屏时间 = 一半
      uint8_t ttime_setting[1];
      SettingGet(ttime_setting, 0x12, 1);
      if(ttime_setting[0] >= 5 && ttime_setting[0] <= 60)
      {
        ui_TTimeValue = ttime_setting[0];
        ui_LTimeValue = ui_TTimeValue / 2;   // 暗屏 = 超时一半
        if(ui_LTimeValue < 5) ui_LTimeValue = 5;  // 最少 5s 暗屏
      }

      // 读取步数
      uint8_t recbuf[3];
      SettingGet(recbuf, 0x20, 3);

      RTC_DateTypeDef nowdate;
      HAL_RTC_GetDate(&hrtc, &nowdate, RTC_FORMAT_BIN);

      if(recbuf[0] == nowdate.Date)
      {
        // 同一天: 从EEPROM恢复步数
        uint16_t steps = (recbuf[1] << 8) | recbuf[2];
        if(!HWInterface.IMU.ConnectionError)
        {
          dmp_set_pedometer_step_count((unsigned long)steps);
        }
      }
      else
      {
        // 新的一天: 步数清零
        if(!HWInterface.IMU.ConnectionError)
        {
          dmp_set_pedometer_step_count(0);
        }
      }
    }

    // AHT21 温湿度传感器
    num = 3;
    while(num && HWInterface.AHT21.ConnectionError)
    {
      num--;
      HWInterface.AHT21.ConnectionError = HWInterface.AHT21.Init();
    }

    // SPL06 气压计
    num = 3;
    while(num && HWInterface.Barometer.ConnectionError)
    {
      num--;
      HWInterface.Barometer.ConnectionError = HWInterface.Barometer.Init();
    }

    // touch
    CST816_GPIO_Init();
    CST816_RESET();

    // LCD init
    LCD_Init();
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    delay_ms(10);
    LCD_Open_Light();
    LCD_Set_Light(80);
    LCD_ShowString(60, 120, (uint8_t*)"LCD OK!", WHITE, BLACK, 24, 0);
    LCD_ShowString(34, 160, (uint8_t*)"OV-Watch V2.4.5", WHITE, BLACK, 16, 0);
    delay_ms(500);
    LCD_Fill(0, LCD_H/2-24, LCD_W, LCD_H/2+49, BLACK);

    // LVGL init
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    ui_init();

    xTaskResumeAll();
		vTaskDelete(NULL);
		
	}
}
