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

#include "streamSerial.h"

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include "lwip.h"

extern UART_HandleTypeDef huart3;
extern osSemaphoreId uartTx3SemaphoreIrqHandle;
extern osSemaphoreId uartRx3SemaphoreIrqHandle;
extern osSemaphoreId uartTx3SemaphoreHandle;
extern osSemaphoreId uartRx3SemaphoreHandle;


struct CmdState cliState;

FILE *cliStream;

CliExRes_t funFoo(CliState_t *state)
{
	fprintf(state->myStdInOut, "Bar\r\n");
	return OK_SILENT;
}

Command_t commands[] =
{
		{"Foo", "Examplary Foo function", funFoo, 1},
		{NULL, NULL, NULL, 0}
};


void StartCliTask(void const * argument)
{
  /* USER CODE BEGIN StartCliTask */
  /* Infinite loop */

  cliStream = openSerialStream(&huart3, uartTx3SemaphoreIrqHandle, uartRx3SemaphoreIrqHandle, uartTx3SemaphoreHandle, uartRx3SemaphoreHandle);
  cmdStateConfigure(&cliState, cliStream, commands, NR_NORMAL);

  //uint8_t dta;
  //HAL_UART_Transmit_IT(&huart3,  "To jest test", 13);


  fprintf(cliStream, "Restart\r\n");
  fflush(cliStream);


  int iter = 0;
  for(;;)
  {
	int x = fgetc(cliStream);
	if (x == -1)
		continue;

    cmdlineInputFunc(x, &cliState);
    cliMainLoop(&cliState);
	fflush(cliStream);

//	fputc(x, cliStream);
	HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	continue;

	osDelay(500);
	fprintf(cliStream, "Test nr %03d\r\n", iter++);
	fflush(cliStream);

	continue;
  }
  /* USER CODE END StartCliTask */
}

