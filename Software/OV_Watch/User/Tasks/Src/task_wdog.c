/* Private includes -----------------------------------------------------------*/
#include "user_TasksInit.h"
#include "task_wdog.h"
#include "WDOG.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile uint8_t wdog_checkin = 0;

/* Private function prototypes -----------------------------------------------*/

void WDOG_CheckIn(uint8_t channel)
{
  wdog_checkin |= channel;
}

void WDOGFeedTask(void *argument)
{
  // Phase 1: Wait for all tasks to check in
  while((wdog_checkin & WDOG_CH_ALL) != WDOG_CH_ALL)
  {
    osDelay(100);
  }

  // Phase 2: All tasks alive, now init GPIO and enable watchdog
  WDOG_Port_Init();
  WDOG_Enable();

  // Phase 3: Feeding loop (LED blinks to indicate watchdog is running)
  while(1)
  {
    wdog_checkin = 0;
    osDelay(500);
    if((wdog_checkin & WDOG_CH_ALL) == WDOG_CH_ALL)
    {
      WDOG_Feed();
     
    }
  }
}
