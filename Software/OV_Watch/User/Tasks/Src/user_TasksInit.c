/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
//sys
#include "stdio.h"
#include "lcd.h"
//gui
#include "ui.h"
#include "PageManager.h"
#include "ui_Charging_Page.h"
//tasks
#include "user_HardwareInitTask.h"
#include "user_RunModeTasks.h"
#include "user_SensUpdateTask.h"
#include "user_KeyTask.h"
#include "user_ScrRenewTask.h"
#include "user_DataSaveTask.h"
#include "user_ChargCheckTask.h"
#include "user_MessageSendTask.h"
#include "task_wdog.h"
#include "WDOG.h"

/* Private typedef -----------------------------------------------------------*/


/* Private define ------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/


/* Timers --------------------------------------------------------------------*/
osTimerId_t IdleTimerHandle;


/* Tasks ---------------------------------------------------------------------*/
// Hardwares initialization
osThreadId_t HardwareInitTaskHandle;
const osThreadAttr_t HardwareInitTask_attributes = {
  .name = "HardwareInitTask",
  .stack_size = 128 * 30,
  .priority = (osPriority_t) osPriorityHigh3,
};

//LVGL Handler task
osThreadId_t LvHandlerTaskHandle;
const osThreadAttr_t LvHandlerTask_attributes = {
  .name = "LvHandlerTask",
  .stack_size = 128 * 64,
  .priority = (osPriority_t) osPriorityLow,
};

//Power management task
osThreadId_t PowerMgrTaskHandle;
const osThreadAttr_t PowerMgrTask_attributes = {
  .name = "PowerMgrTask",
  .stack_size = 128 * 16,
  .priority = (osPriority_t) osPriorityHigh,
};

//Key task
osThreadId_t KeyTaskHandle;
const osThreadAttr_t KeyTask_attributes = {
  .name = "KeyTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

//Screen renew task
osThreadId_t ScrRenewTaskHandle;
const osThreadAttr_t ScrRenewTask_attributes = {
  .name = "ScrRenewTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow1,
};

//Sensor data task
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 128 * 10,
  .priority = (osPriority_t) osPriorityLow1,
};

//Data save task
osThreadId_t DataSaveTaskHandle;
const osThreadAttr_t DataSaveTask_attributes = {
  .name = "DataSaveTask",
  .stack_size = 128 * 5,
  .priority = (osPriority_t) osPriorityLow2,
};

//Watchdog feed task
osThreadId_t WDOGFeedTaskHandle;
const osThreadAttr_t WDOGFeedTask_attributes = {
  .name = "WDOGFeedTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

//Charge check task
osThreadId_t ChargCheckTaskHandle;
const osThreadAttr_t ChargCheckTask_attributes = {
  .name = "ChargCheckTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow2,
};

//BLE message send task
osThreadId_t MessageSendTaskHandle;
const osThreadAttr_t MessageSendTask_attributes = {
  .name = "MessageSendTask",
  .stack_size = 128 * 5,
  .priority = (osPriority_t) osPriorityLow1,
};


/* Message queues ------------------------------------------------------------*/
osMessageQueueId_t Key_MessageQueue;
osMessageQueueId_t Idle_MessageQueue;
osMessageQueueId_t Stop_MessageQueue;
osMessageQueueId_t IdleBreak_MessageQueue;
osMessageQueueId_t HomeUpdata_MessageQueue;
osMessageQueueId_t DataSave_MessageQueue;
osMessageQueueId_t PageCmd_MessageQueue;
osEventFlagsId_t HardIntEventHandle;


/* Private function prototypes -----------------------------------------------*/
static void LvHandlerTask(void *argument);

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void User_Tasks_Init(void)
{
  /* add mutexes, ... */

  /* add semaphores, ... */

  /* start timers, add new ones, ... */
	IdleTimerHandle = osTimerNew(IdleTimerCallback, osTimerPeriodic, NULL, NULL);
	osTimerStart(IdleTimerHandle, 100); // 100ms

  /* add queues, ... */
	Key_MessageQueue = osMessageQueueNew(1, 1, NULL);
	Idle_MessageQueue = osMessageQueueNew(1, 1, NULL);
	Stop_MessageQueue = osMessageQueueNew(1, 1, NULL);
	IdleBreak_MessageQueue = osMessageQueueNew(1, 1, NULL);
	HomeUpdata_MessageQueue = osMessageQueueNew(1, 1, NULL);
	DataSave_MessageQueue = osMessageQueueNew(2, 1, NULL);
	PageCmd_MessageQueue = osMessageQueueNew(2, 1, NULL);
  HardIntEventHandle = osEventFlagsNew(NULL);

	/* add threads, ... */
  HardwareInitTaskHandle = osThreadNew(HardwareInitTask, NULL, &HardwareInitTask_attributes);
  LvHandlerTaskHandle = osThreadNew(LvHandlerTask, NULL, &LvHandlerTask_attributes);
  PowerMgrTaskHandle = osThreadNew(PowerMgrTask, NULL, &PowerMgrTask_attributes);
	KeyTaskHandle = osThreadNew(KeyTask, NULL, &KeyTask_attributes);
	ScrRenewTaskHandle = osThreadNew(ScrRenewTask, NULL, &ScrRenewTask_attributes);
	SensorTaskHandle = osThreadNew(SensorTask, NULL, &SensorTask_attributes);
	DataSaveTaskHandle = osThreadNew(DataSaveTask, NULL, &DataSaveTask_attributes);
	WDOGFeedTaskHandle = osThreadNew(WDOGFeedTask, NULL, &WDOGFeedTask_attributes);
	ChargCheckTaskHandle = osThreadNew(ChargCheckTask, NULL, &ChargCheckTask_attributes);
  MessageSendTaskHandle = osThreadNew(MessageSendTask, NULL, &MessageSendTask_attributes);

  /* add events, ... */

	/* add others ... */
	uint8_t HomeUpdataStr;
	osMessageQueuePut(HomeUpdata_MessageQueue, &HomeUpdataStr, 0, 1);

	/* enable watchdog after all tasks are created */
	// WDOG_Enable();  // 由 WDOGFeedTask 在首次签到后启用
}


/**
  * @brief  FreeRTOS Tick Hook, to increase the LVGL tick
  * @param  None
  * @retval None
  */
//void TaskTickHook(void);


/**
  * @brief  LVGL Handler task, to run the lvgl
  * @param  argument: Not used
  * @retval None
  */
static void LvHandlerTask(void *argument)
{
  uint8_t IdleBreakstr = 0;
  uint8_t pageCmd = 0;
  while(1)
  {
    if(lv_disp_get_inactive_time(NULL) < 1000)
    {
      osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 0);
    }
    if(osMessageQueueGet(PageCmd_MessageQueue, &pageCmd, NULL, 0) == osOK)
    {
      if(pageCmd == 1)
        Page_Back(LV_SCR_LOAD_ANIM_NONE, 0, 0);
      else if(pageCmd == 2)
        Page_Back_Home();
      else if(pageCmd == 3)
        Page_Load(&Page_Charging,LV_SCR_LOAD_ANIM_NONE, 100, 0);
    }
    WDOG_CheckIn(WDOG_CH_LVGL);
    lv_task_handler();
    osDelay(1);
  }
}





