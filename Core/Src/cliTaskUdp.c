#include <stdio.h>

#include "cliTaskUdp.h"
#include "streamUdp.h"
#include "vty.h"


void StartCliTaskUdp(void const * argument)
{
	osDelay(1000);

	FILE *cliUdpStreamIn;
	FILE *cliUdpStreamOut;
	struct CmdState cliUdpState;


	ip_addr_t PC_IPADDR;
	IP_ADDR4(&PC_IPADDR, 192, 168, 1, 120);

	openUdpStreams(&cliUdpStreamIn, &cliUdpStreamOut, PC_IPADDR, 55151);
	cmdStateConfigure(&cliUdpState, cliUdpStreamIn, cliUdpStreamOut, cmdListNormal, NR_NORMAL);

	for(;;)
	{
		int x = fgetc(cliUdpStreamIn);
		if (x == -1)
			continue;

		cmdlineInputFunc(x, &cliUdpState);
		cliMainLoop(&cliUdpState);
		fflush(cliUdpStreamOut);
	}
	/* USER CODE END StartCliTask */
}
