/* Private includes -----------------------------------------------------------*/
#include "user_TasksInit.h"
#include "user_ScrRenewTask.h"
#include "main.h"
#include "lvgl.h"
#include "../Inc/ui_Home_Page.h"
#include "../Inc/ui_Charging_Page.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/**
 * @brief  Screen renew task
 * @param  argument: Not used
 * @retval None
 */
void ScrRenewTask(void *argument)
{
	uint8_t keystr = 0;
	uint8_t pageCmd = 0;
	while (1)
	{
		if (osMessageQueueGet(Key_MessageQueue, &keystr, NULL, 0) == osOK)
		{
			if (keystr == 1)
			{
				pageCmd = 1;
				osMessageQueuePut(PageCmd_MessageQueue, &pageCmd, 0, 0);
			}
			else if (keystr == 2)
			{
				pageCmd = 2;
				osMessageQueuePut(PageCmd_MessageQueue, &pageCmd, 0, 0);
			}
		}
		if (osMessageQueueGet(PageCmd_MessageQueue, &pageCmd, NULL, 0) == osOK)
		{
			if (pageCmd == 1)
				Page_Back(LV_SCR_LOAD_ANIM_NONE, 0, 0);
			else if (pageCmd == 2)
				Page_Back_Home();
			else if (pageCmd == 3)
				Page_Load(&Page_Charging, LV_SCR_LOAD_ANIM_NONE, 100, 0);
		}
		osDelay(10);
	}
}
