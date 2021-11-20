#include <stdio.h>

#include "cliTaskTcp.h"
#include "cmdline.h"
#include "streamTcp.h"
#include "vty.h"


osThreadId tcpTasks[NO_OF_TCP_SERVER_TASKS];

static void StartCliTaskTcp(void const * argument);


void StartTcpServer()
{
	for (int i=0; i < NO_OF_TCP_SERVER_TASKS; i++)
	{
		osThreadDef(tmpTask, StartCliTaskTcp, osPriorityNormal, 0, 256);
		tcpTasks[i] = osThreadCreate(osThread(tmpTask), NULL);
	}
	startTcpServer(55151, tcpTasks);
}

static void StartCliTaskTcp(void const * argument)
{
	osDelay(1000);

	FILE *cliTcpStreamIn;
	FILE *cliTcpStreamOut;
	struct CmdState cliTcpState;

	//Listen


	for(;;)
	{
		acceptTcpConnection(&cliTcpStreamIn, &cliTcpStreamOut, argument);
		cmdStateConfigure(&cliTcpState, cliTcpStreamIn, cliTcpStreamOut, cmdListNormal, NR_NORMAL);

		for (;;)
		{
			int x = fgetc(cliTcpStreamIn);
			if (x == -1)
				break;

			cmdlineInputFunc(x, &cliTcpState);
			cliMainLoop(&cliTcpState);
			fflush(cliTcpStreamOut);
		}
	}
	/* USER CODE END StartCliTask */
}

