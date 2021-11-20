#ifndef INC_STREAM_TCP_H_
#define INC_STREAM_TCP_H_

#include <stdio.h>

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"

#include "lwip/ip_addr.h"

void startTcpServer(uint16_t portNo);

int acceptTcpConnection(FILE** stramIn, FILE** streamOut, int taskNo);

struct StreamTcpHandler
{
	struct tcp_pcb *my_tcp;

/// Receiver
	osSemaphoreId   rxSemaphoreHandle;
	osSemaphoreId   rxIrqSemaphoreHandle;
	struct pbuf    *recBuffer;
	uint16_t        recBufRdPos;


/// Transmitter
	osSemaphoreId   txSemaphoreHandle;
	struct pbuf    *txBuffer;
};


#endif /* INC_STREAM_TCP_H_ */
