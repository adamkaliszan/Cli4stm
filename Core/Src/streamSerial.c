/*
 * streamSerial.c
 *
 *  Created on: Apr 11, 2021
 *      Author: adam
 */

#include <string.h>

#include "streamSerial.h"


static ssize_t dummy_cookie_write(void *cookie, const char *buf, unsigned int remainingSize);
static ssize_t dummy_cookie_read(void *cookie, char *buf, unsigned int remainingSize);

#ifndef NO_OF_SERIAL_UART_HANDLERS
#define NO_OF_SERIAL_UART_HANDLERS 1
#endif

static struct StreamSerialHandler StramSerialHandlers[NO_OF_SERIAL_UART_HANDLERS];
static int noOfSerialHandlers = 0;

cookie_io_functions_t dummy_cookie_funcs = {
  .read = dummy_cookie_read,
  .write = dummy_cookie_write,
  .seek = 0,
  .close = 0
};

FILE* openSerialStream(UART_HandleTypeDef *uartHandle, osSemaphoreId uartTxSemaphoreHandle, osSemaphoreId uartTxIrqSemaphoreHandle, osSemaphoreId uartRxSemaphoreHandle, osSemaphoreId uartRxIrqSemaphoreHandle)
{
	if (noOfSerialHandlers >= NO_OF_SERIAL_UART_HANDLERS)
		return NULL;

	StramSerialHandlers[noOfSerialHandlers].uartTxSemaphoreHandle    = uartTxSemaphoreHandle;
	StramSerialHandlers[noOfSerialHandlers].uartTxSemaphoreIrqHandle = uartTxIrqSemaphoreHandle;
	StramSerialHandlers[noOfSerialHandlers].uartRxSemaphoreHandle    = uartRxSemaphoreHandle;
	StramSerialHandlers[noOfSerialHandlers].uartRxSemaphoreIrqHandle = uartRxIrqSemaphoreHandle;
	StramSerialHandlers[noOfSerialHandlers].rxDtaSize                = 0;

	return fopencookie(&StramSerialHandlers[noOfSerialHandlers++], "r+", dummy_cookie_funcs);
}


static ssize_t dummy_cookie_read(void *cookie, char *buf, unsigned int remainingSize)
{
	struct StreamSerialHandler *serialHandler = (struct StreamSerialHandler *)cookie;

	int result = 0;

    if (osSemaphoreWait(serialHandler->uartRxSemaphoreHandle, 10000) != osOK)
	    goto exit;

    while (remainingSize > 0)
    {
    	if (osSemaphoreWait(serialHandler->uartRxSemaphoreIrqHandle, 100) != osOK)
    		goto exit2;

    	HAL_UARTEx_ReceiveToIdle_IT(serialHandler->device, (uint8_t *)buf, remainingSize);

    	if (osSemaphoreWait(serialHandler->uartRxSemaphoreIrqHandle, 100) != osOK)
    		goto exit2;

        remainingSize-= serialHandler->rxDtaSize;
        buf+= serialHandler->rxDtaSize;
        result+= serialHandler->rxDtaSize;

    	osSemaphoreRelease(serialHandler->uartRxSemaphoreIrqHandle);
    }
exit2:
    osSemaphoreRelease(serialHandler->uartRxSemaphoreHandle);
exit:
	return result;
}


static ssize_t dummy_cookie_write(void *cookie, const char *buf, unsigned int remainingSize)
{
	struct StreamSerialHandler *serialHandler = (struct StreamSerialHandler *)cookie;
	int result = 0;
	int blockSize;

	if ((result = osSemaphoreWait(serialHandler->uartTxSemaphoreHandle, 1000)) != osOK) goto exit;

	result = 0; //In optimization process this line schould be removed
    do
    {
    	blockSize = sizeof(serialHandler->tmpTxBuffer);
        if (remainingSize < blockSize)
        {
        	blockSize = remainingSize;
        }

    	if (osSemaphoreWait(serialHandler->uartTxSemaphoreIrqHandle, 100) != osOK) goto exit;
    	memcpy(serialHandler->tmpTxBuffer, buf, blockSize);
    	HAL_UART_Transmit_IT(serialHandler->device,  serialHandler->tmpTxBuffer, blockSize);

        buf+= blockSize;
    	remainingSize-= blockSize;
    	result+= blockSize;
    }
    while (remainingSize > 0);
	osSemaphoreRelease(serialHandler->uartTxSemaphoreHandle);

exit:
    return result;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	struct StreamSerialHandler *serialHandler = StramSerialHandlers;
	for (int i=0; i < noOfSerialHandlers; i++, serialHandler++)
	{
		if (huart != serialHandler->device)
			continue;
		osSemaphoreRelease(serialHandler->uartRxSemaphoreIrqHandle);
		serialHandler -> rxDtaSize = Size;
		break;
	}
}

/*
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart3)
	{
		osSemaphoreRelease(uartRxSemaphoreIrqHandle);

//		BaseType_t hptw = pdFALSE;
//		xQueueSendFromISR(queueRxUart3Handle, &_rxDta, NULL);
//		HAL_UART_Receive_IT(huart, &_rxDta, 1);
//		if (hptw == pdTRUE)
//			osThreadYield();
	}
}
*/

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	struct StreamSerialHandler *serialHandler = StramSerialHandlers;
	for (int i=0; i < noOfSerialHandlers; i++, serialHandler++)
	{
		if (huart != serialHandler->device)
			continue;
		osSemaphoreRelease(serialHandler->uartTxSemaphoreIrqHandle);
		break;
	}
}
