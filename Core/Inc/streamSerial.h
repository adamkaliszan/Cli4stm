/*
 * streamSerial.h
 *
 *  Created on: Apr 11, 2021
 *      Author: adam
 */

#ifndef INC_STREAM_SERIAL_H_
#define INC_STREAM_SERIAL_H_

#include <stdio.h>

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"


FILE* openSerialStream(UART_HandleTypeDef *uartHandle);


struct StreamSerialHandler
{
	UART_HandleTypeDef *device;

	osSemaphoreId uartTxSemaphoreHandle;
	osSemaphoreId uartTxSemaphoreIrqHandle;
	osSemaphoreId uartRxSemaphoreHandle;
	osSemaphoreId uartRxSemaphoreIrqHandle;

	volatile uint16_t rxDtaSize;

	uint8_t tmpTxBuffer[64];
};

#endif /* INC_STREAM_SERIAL_H_ */
