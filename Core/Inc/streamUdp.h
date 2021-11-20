#ifndef INC_STREAM_UDP_H_
#define INC_STREAM_UDP_H_

#include <stdio.h>

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"

#include "lwip/ip_addr.h"

int openUdpStreams(FILE **streamIn, FILE **streamOut, ip_addr_t clientAddr, uint16_t portNo);

struct StreamUdpHandler
{
	struct udp_pcb *my_udp;
	ip_addr_t       clientAddr;
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


#endif /* INC_STREAM_UDP_H_ */
