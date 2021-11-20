#include <stdio.h>

#include "cliTaskTcp.h"
#include "cmdline.h"
#include "streamTcp.h"
#include "vty.h"

void StartCliTaskTcp(void const * argument)
{
	osDelay(1000);

	FILE *cliTcpStreamIn;
	FILE *cliTcpStreamOut;
	struct CmdState cliTcpState;

	//Listen

	startTcpServer(55151);

	for(;;)
	{
		acceptTcpConnection(&cliTcpStreamIn, &cliTcpStreamOut, 0);
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

