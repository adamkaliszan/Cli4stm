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


	openTcpStreams(&cliTcpStreamIn, &cliTcpStreamOut, 55151);
	cmdStateConfigure(&cliTcpState, cliTcpStreamIn, cliTcpStreamOut, cmdListNormal, NR_NORMAL);

	//Listen

	for(;;)
	{
		int x = fgetc(cliTcpStreamIn);
		if (x == -1)
			continue;

		cmdlineInputFunc(x, &cliTcpState);
		cliMainLoop(&cliTcpState);
		fflush(cliTcpStreamOut);
	}
	/* USER CODE END StartCliTask */
}
