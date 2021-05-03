/*
 * streamSerial.h
 *
 *  Created on: Apr 11, 2021
 *      Author: adam
 */

#ifndef INC_STREAM_UDP_H_
#define INC_STREAM_UDP_H_

#include <stdio.h>

#include "lwip/ip_addr.h"

#include "cliTask.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "cmdline.h"

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"


FILE* openUdpStream(ip_addr_t bindAddress, ip_addr_t clientAddr, uint16_t portNo);


struct StreamUdpHandler
{
	struct udp_pcb *my_udp;
	ip_addr_t       clientAddr;
	uint16_t        dstPort;
	//uint16_t        srcPort;


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
