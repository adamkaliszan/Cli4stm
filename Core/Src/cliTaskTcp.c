#include <stdio.h>

#include "cliTaskTcp.h"
#include "cmdline.h"
#include "streamTcp.h"
#include "vty.h"


osThreadId tcpTasks[NO_OF_TCP_SERVER_TASKS];

static void _cliTaskLoop(void const * argument);


void StartCliTcpServer()
{
	for (int i=0; i < NO_OF_TCP_SERVER_TASKS; i++)
	{
		osThreadDef(tmpTask, _cliTaskLoop, osPriorityNormal, 0, 256);
		tcpTasks[i] = osThreadCreate(osThread(tmpTask), NULL);
	}
	startTcpServer(55151, tcpTasks);
}

static void _cliTaskLoop(void const * argument)
{
	(void) argument;
	static int taskIdx = 0;

	int myTaskIdx = taskIdx++;
	FILE *cliTcpStreamIn;
	FILE *cliTcpStreamOut;
	struct CmdState cliTcpState;

	acceptTcpConnection(&cliTcpStreamIn, &cliTcpStreamOut, myTaskIdx);
	cmdStateConfigure(&cliTcpState, cliTcpStreamIn, cliTcpStreamOut, cmdListNormal, NR_NORMAL);

	for (;;)
	{
//		osDelay(1000);
//		continue;
		int x = fgetc(cliTcpStreamIn);
		if (x == -1)
		{
			continue;
		}
		cmdlineInputFunc(x, &cliTcpState);
		cliMainLoop(&cliTcpState);
		fflush(cliTcpStreamOut);
	}
	/* USER CODE END StartCliTask */
}

