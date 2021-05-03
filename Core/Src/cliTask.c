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
#include "streamUdp.h"

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"

#include "lwip.h"
#include "udp.h"


extern UART_HandleTypeDef huart3;


struct CmdState cliSerialState;
struct CmdState cliUdpState;


FILE *cliSerialStream;
FILE *cliUdpStream;


void StartCliTask(void const * argument)
{
  /* USER CODE BEGIN StartCliTask */
  /* Infinite loop */

  cliSerialStream = openSerialStream(&huart3);
  fprintf(cliSerialStream, "Restart\r\n");
  fflush(cliSerialStream);

  cmdStateConfigure(&cliSerialState, cliSerialStream, cmdListNormal, NR_NORMAL);

  cmdlineInputFunc('\r', &cliSerialState);
  cliMainLoop(&cliSerialState);
  fflush(cliSerialStream);

  for(;;)
  {
	int x = fgetc(cliSerialStream);
	if (x == -1)
		continue;

    cmdlineInputFunc(x, &cliSerialState);
    cliMainLoop(&cliSerialState);
	fflush(cliSerialStream);
  }
  /* USER CODE END StartCliTask */
}

void StartUdpTask(void const * argument)
{
  osDelay(1000);

  ip_addr_t PC_IPADDR;
  ip_addr_t PC_IPADDR_MASK;


  IP_ADDR4(&PC_IPADDR, 192, 168, 1, 120);

  IP_ADDR4(&PC_IPADDR_MASK, 0, 0, 0, 0);


  cliUdpStream = openUdpStream(PC_IPADDR_MASK, PC_IPADDR, 55151);
  fprintf(cliUdpStream, "Restart\r\n");
  fflush(cliUdpStream);

  cmdStateConfigure(&cliUdpState, cliUdpStream, cmdListNormal, NR_NORMAL);

  cmdlineInputFunc('\r', &cliUdpState);
  cliMainLoop(&cliUdpState);
  fflush(cliUdpStream);

  for(;;)
  {
	int x = fgetc(cliUdpStream);
	if (x == -1)
		continue;

    cmdlineInputFunc(x, &cliUdpState);
    cliMainLoop(&cliUdpState);
	fflush(cliUdpStream);
  }
  /* USER CODE END StartCliTask */
}

