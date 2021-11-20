#ifndef INC_STREAM_TCP_H_
#define INC_STREAM_TCP_H_

#include <stdio.h>

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"

#include "lwip/ip_addr.h"

int openTcpStreams(FILE** stramIn, FILE** streamOut, uint16_t portNo);

struct StreamTcpHandler
{
	struct tcp_pcb *my_tcp;
	uint16_t        dstPort;

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
