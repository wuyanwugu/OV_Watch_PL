/* Private includes -----------------------------------------------------------*/
#include "user_TasksInit.h"
#include "user_DataSaveTask.h"
#include "main.h"
#include "rtc.h"
#include "DataSave.h"
#include "HWDataAccess.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern uint8_t ui_LTimeValue;

/******************************************
EEPROM Data description:
[0x00]:0x55 for check
[0x01]:0xAA for check

[0x10]:wrist_is_enabled
[0x12]:ui_LTimeValue
[0x20]:Last Save Day(0-31)
[0x21]:Day Steps High
[0x22]:Day Steps Low
******************************************/

/* Private function prototypes -----------------------------------------------*/

/* Tasks ---------------------------------------------------------------------*/

/**
  * @brief  Data Save in the EEPROM
  * @param  argument: Not used
  * @retval None
  */
void DataSaveTask(void *argument)
{
  while(1)
  {
    uint8_t Datastr = 0;
    if(osMessageQueueGet(DataSave_MessageQueue, &Datastr, NULL, 1) == osOK)
    {
      // Save wrist_is_enabled setting
      uint8_t wrist_dat[1];
      wrist_dat[0] = HWInterface.IMU.wrist_is_enabled;
      SettingSave(wrist_dat, 0x10, 1);

      // Save ui_LTimeValue setting
      uint8_t ltime_dat[1];
      ltime_dat[0] = ui_LTimeValue;
      SettingSave(ltime_dat, 0x12, 1);

      uint8_t dat[3];

      RTC_DateTypeDef nowdate;
      HAL_RTC_GetDate(&hrtc, &nowdate, RTC_FORMAT_BIN);

      // Check if it's a new day
      uint8_t saved_day[1];
      SettingGet(saved_day, 0x20, 1);

      if(saved_day[0] != nowdate.Date)
      {
        // New day: reset steps
        if(!HWInterface.IMU.ConnectionError)
        {
          HWInterface.IMU.SetSteps(0);
        }
      }

      // Save current day and steps
      uint16_t steps = HWInterface.IMU.GetSteps();
      dat[0] = nowdate.Date;
      dat[1] = (steps >> 8) & 0xFF;
      dat[2] = steps & 0xFF;
      SettingSave(dat, 0x20, 3);
    }
    osDelay(100);
  }
}
