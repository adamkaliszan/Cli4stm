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

