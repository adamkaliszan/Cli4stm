/*
 * streamSerial.c
 *
 *  Created on: Apr 11, 2021
 *      Author: Adam Kaliszan: adam.kaliszan@gmail.com
 */

#include <stdio.h>
#include <string.h>

#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"

#include "streamUdp.h"

static void udpStreamRecHandler (void *cookie, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

static ssize_t dummy_cookie_write(void *cookie, const char *buf, unsigned int remainingSize);
static ssize_t dummy_cookie_read(void *cookie, char *buf, unsigned int remainingSize);

#ifndef NO_OF_UDP_STREAM_HANDLERS
#define NO_OF_UDP_STREAM_HANDLERS 1
#endif

static struct StreamUdpHandler StreamUdpHandlers[NO_OF_UDP_STREAM_HANDLERS];
volatile static int noOfUdpHandlers = 0;

cookie_io_functions_t dummy_udp_cookie_funcs = {
  .read = dummy_cookie_read,
  .write = dummy_cookie_write,
  .seek = 0,
  .close = 0
};



int openUdpStreams(FILE **streamIn, FILE **streamOut, ip_addr_t clientAddr, uint16_t portNo)
{
	if (noOfUdpHandlers >= NO_OF_UDP_STREAM_HANDLERS)
	{
		*streamIn = NULL;
		*streamOut= NULL;
		return -1;
	}

	StreamUdpHandlers[noOfUdpHandlers].my_udp     = udp_new();
	StreamUdpHandlers[noOfUdpHandlers].clientAddr = clientAddr;
	//StreamUdpHandlers[noOfUdpHandlers].srcPort = 	portNo;
	StreamUdpHandlers[noOfUdpHandlers].dstPort = 	portNo;

	udp_bind(StreamUdpHandlers[noOfUdpHandlers].my_udp, NULL, portNo);
	StreamUdpHandlers[noOfUdpHandlers].txBuffer   = NULL;

	osSemaphoreDef_t tmp = {.controlblock = NULL};
	StreamUdpHandlers[noOfUdpHandlers].txSemaphoreHandle    = osSemaphoreCreate(&tmp, 1);
	StreamUdpHandlers[noOfUdpHandlers].rxSemaphoreHandle    = osSemaphoreCreate(&tmp, 1);
	StreamUdpHandlers[noOfUdpHandlers].rxIrqSemaphoreHandle = osSemaphoreCreate(&tmp, 1);

	*streamIn = fopencookie(&StreamUdpHandlers[noOfUdpHandlers], "r", dummy_udp_cookie_funcs);
	*streamOut= fopencookie(&StreamUdpHandlers[noOfUdpHandlers], "w", dummy_udp_cookie_funcs);
	noOfUdpHandlers++;

	return 0;
}


static ssize_t procUdpBuffer(struct StreamUdpHandler *udpHandler, char *buf, unsigned int size)
{
	ssize_t result = 0;
	ssize_t blSize;
	struct pbuf *nextRecBuffer;
	while (result < size)
	{
		blSize = udpHandler->recBuffer->len - udpHandler->recBufRdPos;

		if (result + blSize <= size)
		{
			memcpy(buf, udpHandler->recBuffer->payload + udpHandler->recBufRdPos, blSize);
			result+= blSize;

			nextRecBuffer = udpHandler->recBuffer->next;
			pbuf_free(udpHandler->recBuffer);
			udpHandler->recBuffer = nextRecBuffer;
			udpHandler->recBufRdPos = 0;
			if (nextRecBuffer == NULL)
				break;
		}
		else
		{
			blSize = size-result;

			memcpy(buf, udpHandler->recBuffer->payload + udpHandler->recBufRdPos, blSize);
			udpHandler->recBufRdPos+= blSize;
			result+= blSize;
		}
	}
	return result;
}

static ssize_t dummy_cookie_read(void *cookie, char *buf, unsigned int size)
{
	struct StreamUdpHandler *udpHandler = (struct StreamUdpHandler *)cookie;

	ssize_t result = 0;

    if (osSemaphoreWait(udpHandler->rxSemaphoreHandle, 10000) != osOK)
	    goto exit;


    if (udpHandler->recBuffer != NULL)
    {
    	result = procUdpBuffer(udpHandler, buf, size);
    }
    else
    {
      	osSemaphoreWait(udpHandler->rxIrqSemaphoreHandle, 0); //TODO use signals

       	udp_recv(udpHandler->my_udp, udpStreamRecHandler, cookie);

       	while (osSemaphoreWait(udpHandler->rxIrqSemaphoreHandle, 1000) != osOK)
       		;

       	result = procUdpBuffer(udpHandler, buf, size);

       	osSemaphoreRelease(udpHandler->rxIrqSemaphoreHandle);
    }
    osSemaphoreRelease(udpHandler->rxSemaphoreHandle);


exit:
	return result;
}

static ssize_t dummy_cookie_write(void *cookie, const char *buf, unsigned int size)
{
  struct StreamUdpHandler *udpHandler = (struct StreamUdpHandler *)cookie;

  if (osSemaphoreWait(udpHandler->txSemaphoreHandle, 10000) != osOK)
  {
    size = 0;
	goto exit;
  }


  //udpHandler->txBuffer = pbuf_alloc_reference((void *)buf, size, PBUF_REF); //TODO ugly const casting .... RAM_D1 ???
  udpHandler->txBuffer = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);
  if (udpHandler->txBuffer != NULL)
  {
     memcpy(udpHandler->txBuffer->payload, buf, size);
     udp_sendto(udpHandler->my_udp, udpHandler->txBuffer, &udpHandler->clientAddr, udpHandler->dstPort);
     pbuf_free(udpHandler->txBuffer);
     udpHandler->txBuffer = NULL;
  }
  else
  {
    size = 0;
  }
  osSemaphoreRelease(udpHandler->txSemaphoreHandle);

exit:
    return size;
}

static void udpStreamRecHandler (void *cookie, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	struct StreamUdpHandler *udpHandler = (struct StreamUdpHandler *)cookie;

	udpHandler->recBuffer = p;
	udpHandler->recBufRdPos = 0;

	udpHandler->clientAddr.addr = addr->addr;
	udpHandler->dstPort = port;

	osSemaphoreRelease(udpHandler->rxIrqSemaphoreHandle);
}
