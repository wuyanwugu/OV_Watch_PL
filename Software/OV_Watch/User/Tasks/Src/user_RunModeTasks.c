/* Private includes -----------------------------------------------------------*/
#include "user_TasksInit.h"
#include "user_RunModeTasks.h"
#include "main.h"
#include "lcd.h"
#include "lcd_init.h"
#include "CST816.h"
#include "key.h"
#include "power.h"
#include "WDOG.h"
#include "rtc.h"
#include "HWDataAccess.h"
#include "PageManager.h"
#include "task_wdog.h"
#include "stm32f4xx_it.h"
#include "../Inc/ui_Brightness_Page.h"
#include "../Inc/ui_Menu_Page.h"
#include "MPU6050.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint16_t IdleTimerCount = 0;
extern uint8_t ui_LTimeValue;  // 亮屏超时时间 (秒)，定义在 user_HardwareInitTask.c

/* Private function prototypes -----------------------------------------------*/
extern void SystemClock_Config(void);

/* Tasks ---------------------------------------------------------------------*/

/**
  * @brief  Power management task
  * @param  argument: Not used
  * @retval None
  */
void PowerMgrTask(void *argument)
{
  uint8_t Idlestr = 0;
  uint8_t IdleBreakstr = 0;
  uint8_t Stopstr = 0;
  uint8_t HomeUpdataStr = 0;

  while(1)
  {
    // Light off on idle
    if(osMessageQueueGet(Idle_MessageQueue, &Idlestr, NULL, 1) == osOK)
    {
      LCD_Set_Light(5);
    }

    // Resume light on activity
    if(osMessageQueueGet(IdleBreak_MessageQueue, &IdleBreakstr, NULL, 1) == osOK)
    {
      IdleTimerCount = 0;
      LCD_Set_Light(brightness);
    }

    // Stop mode entry
    if(osMessageQueueGet(Stop_MessageQueue, &Stopstr, NULL, 0) == osOK)
    {
      uint8_t Wrist_Flag = 0;

      /*************************** 进入休眠前操作 ***************************/
      sleep:
      IdleTimerCount = 0;

      LCD_RES_Clr();
      LCD_Close_Light();
      CST816_Sleep();

    
      /****************************** 进入 Stop 模式 *****************************/
      vTaskSuspendAll();
      WDOG_Disnable();
      CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
      HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);

      // ====== 休眠中 ======

      /****************************** 退出 Stop 模式 *****************************/
      SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
      HAL_SYSTICK_Config(SystemCoreClock / (1000U / uwTickFreq));
      SystemClock_Config();
      WDOG_Feed();
      xTaskResumeAll();

      /****************************** 唤醒判断 *****************************/
      // MPU 抬腕检测
      if(HWInterface.IMU.wrist_is_enabled)
      {
        uint8_t hor = MPU_isHorizontal();
        if(hor && HWInterface.IMU.wrist_state == WRIST_DOWN)
        {
          HWInterface.IMU.wrist_state = WRIST_UP;
          Wrist_Flag = 1;
        }
        else if(!hor && HWInterface.IMU.wrist_state == WRIST_UP)
        {
          HWInterface.IMU.wrist_state = WRIST_DOWN;
          IdleTimerCount = 0;
          goto sleep;
        }
      }
        
      // 判断是否真正唤醒
      if(!KEY1 || KEY2 || ChargeCheck() || Wrist_Flag)
      {
        Wrist_Flag = 0;
        // 继续恢复
      }
      else
      {
        IdleTimerCount = 0;
        goto sleep;
      }

      /****************************** 恢复外设 *****************************/
      LCD_Init();
      LCD_Set_Light(brightness);
      CST816_Wakeup();

      if(ChargeCheck())
      {
        HardInt_Charg_flag = 1;
      }

      // 恢复看门狗
      WDOG_Port_Init();
      WDOG_Enable();

      // 刷新主页数据
      osMessageQueuePut(HomeUpdata_MessageQueue, &HomeUpdataStr, 0, 1);
    }

    WDOG_CheckIn(WDOG_CH_POWER);
    osDelay(10);
  }
}

/**
  * @brief  Idle timer callback (100ms period)
  * @param  argument: Not used
  * @retval None
  */
void IdleTimerCallback(void *argument)
{
  IdleTimerCount += 1;

  // 使用保存的超时时间，若未设置则默认 10 秒
  uint16_t timeout_sec = (ui_LTimeValue >= 5) ? ui_LTimeValue : 10;
  uint16_t dim_ticks    = timeout_sec * 5;   // 一半时间暗屏 (1 tick = 100ms)
  uint16_t stop_ticks   = timeout_sec * 10;  // 超时进入 Stop 模式

  if(IdleTimerCount == dim_ticks)
  {
    uint8_t Idlestr = 0;
    osMessageQueuePut(Idle_MessageQueue, &Idlestr, 0, 1);
  }

  if(IdleTimerCount >= stop_ticks)
  {
    uint8_t Stopstr = 1;
    IdleTimerCount = 0;
    osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 1);
  }
	if(g_Sleep)
	{
		uint8_t Stopstr = 1;
    IdleTimerCount = 0;
		g_Sleep = false;
    osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 1);
	}
}
