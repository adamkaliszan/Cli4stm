/*
 * cliTask.cpp
 *
 *  Created on: Apr 5, 2021
 *      Author: adam
 */

#include <stdio.h>
#include <string.h>

#include "cliTask.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "cmdline.h"
#include "vty.h"

#include "streamSerial.h"

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include "lwip.h"

extern UART_HandleTypeDef huart3;


struct CmdState cliState;

FILE *cliStream;


void StartCliTask(void const * argument)
{
  /* USER CODE BEGIN StartCliTask */
  /* Infinite loop */

  cliStream = openSerialStream(&huart3);
  fprintf(cliStream, "Restart\r\n");
  fflush(cliStream);

  cmdStateConfigure(&cliState, cliStream, cmdListNormal, NR_NORMAL);

  cmdlineInputFunc('\r', &cliState);
  cliMainLoop(&cliState);
  fflush(cliStream);

  for(;;)
  {
	int x = fgetc(cliStream);
	if (x == -1)
		continue;

    cmdlineInputFunc(x, &cliState);
    cliMainLoop(&cliState);
	fflush(cliStream);
  }
  /* USER CODE END StartCliTask */
}

void StartUdpTask(void const * argument)
{
  /* USER CODE BEGIN StartCliTask */
  /* Infinite loop */

	/* USER CODE BEGIN 5 */
	const char* message = "Hello UDP message!\n\r";
	osDelay(1000);

	ip_addr_t PC_IPADDR;
	IP_ADDR4(&PC_IPADDR, 192, 168, 1, 120);

	struct udp_pcb* my_udp = udp_new();
	udp_connect(my_udp, &PC_IPADDR, 55151);
	struct pbuf* udp_buffer = NULL;

  for(;;)
  {
	  osDelay(1000);
	  udp_buffer = pbuf_alloc(PBUF_TRANSPORT, strlen(message), PBUF_RAM);
	  if (udp_buffer != NULL) {
	    memcpy(udp_buffer->payload, message, strlen(message));
	    udp_send(my_udp, udp_buffer);
	    pbuf_free(udp_buffer);
	  }
  }
  /* USER CODE END StartCliTask */
}

