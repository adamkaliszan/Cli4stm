/*
 * streamSerial.c
 *
 *  Created on: Apr 11, 2021
 *      Author: Adam Kaliszan: adam.kaliszan@gmail.com
 */

#include <stdio.h>
#include <string.h>

#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"

#include "streamTcp.h"

static ssize_t dummy_cookie_write(void *cookie, const char *buf, unsigned int remainingSize);
static ssize_t dummy_cookie_read(void *cookie, char *buf, unsigned int remainingSize);

#ifndef NO_OF_TCP_STREAM_HANDLERS
#define NO_OF_TCP_STREAM_HANDLERS 1
#endif

static struct StreamTcpHandler StreamTcpHandlers[NO_OF_TCP_STREAM_HANDLERS];
volatile static int noOfTcpHandlers = 0;

cookie_io_functions_t dummy_tcp_cookie_funcs = {
  .read = dummy_cookie_read,
  .write = dummy_cookie_write,
  .seek = 0,
  .close = 0
};

static err_t tcpRecHandler(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

int openTcpStreams(FILE** streamIn, FILE** streamOut, uint16_t portNo)
{
	if (noOfTcpHandlers >= NO_OF_TCP_STREAM_HANDLERS)
	{
		*streamIn  = NULL;
		*streamOut = NULL;
		return -1;
	}

	StreamTcpHandlers[noOfTcpHandlers].my_tcp  = tcp_new();
	StreamTcpHandlers[noOfTcpHandlers].dstPort = portNo;

	tcp_bind(StreamTcpHandlers[noOfTcpHandlers].my_tcp, NULL, portNo);
	StreamTcpHandlers[noOfTcpHandlers].txBuffer   = NULL;

	osSemaphoreDef_t tmp = {.controlblock = NULL};
	StreamTcpHandlers[noOfTcpHandlers].txSemaphoreHandle    = osSemaphoreCreate(&tmp, 1);
	StreamTcpHandlers[noOfTcpHandlers].rxSemaphoreHandle    = osSemaphoreCreate(&tmp, 1);
	StreamTcpHandlers[noOfTcpHandlers].rxIrqSemaphoreHandle = osSemaphoreCreate(&tmp, 1);

	*streamIn  = fopencookie(&StreamTcpHandlers[noOfTcpHandlers], "r", dummy_tcp_cookie_funcs);
	*streamOut = fopencookie(&StreamTcpHandlers[noOfTcpHandlers], "w", dummy_tcp_cookie_funcs);
	noOfTcpHandlers++;

	return 0;
}

static ssize_t procTcpBuffer(struct StreamTcpHandler *tcpHandler, char *buf, unsigned int size)
{
	ssize_t result = 0;
	ssize_t blSize;
	struct pbuf *nextRecBuffer;
	while (result < size)
	{
		blSize = tcpHandler->recBuffer->len - tcpHandler->recBufRdPos;

		if (result + blSize <= size)
		{
			memcpy(buf, tcpHandler->recBuffer->payload + tcpHandler->recBufRdPos, blSize);
			result+= blSize;

			nextRecBuffer = tcpHandler->recBuffer->next;
			pbuf_free(tcpHandler->recBuffer);
			tcpHandler->recBuffer = nextRecBuffer;
			tcpHandler->recBufRdPos = 0;
			if (nextRecBuffer == NULL)
				break;
		}
		else
		{
			blSize = size-result;

			memcpy(buf, tcpHandler->recBuffer->payload + tcpHandler->recBufRdPos, blSize);
			tcpHandler->recBufRdPos+= blSize;
			result+= blSize;
		}
	}
	return result;
}

static ssize_t dummy_cookie_read(void *cookie, char *buf, unsigned int size)
{
	struct StreamTcpHandler *tcpHandler = (struct StreamTcpHandler *)cookie;

	ssize_t result = 0;

    if (osSemaphoreWait(tcpHandler->rxSemaphoreHandle, 10000) != osOK)
	    goto exit;


    if (tcpHandler->recBuffer != NULL)
    {
    	result = procTcpBuffer(tcpHandler, buf, size);
    }
    else
    {
      	osSemaphoreWait(tcpHandler->rxIrqSemaphoreHandle, 0); //TODO use signals

       	tcp_recv(tcpHandler->my_tcp, tcpRecHandler);

       	while (osSemaphoreWait(tcpHandler->rxIrqSemaphoreHandle, 1000) != osOK)
       		;

       	result = procTcpBuffer(tcpHandler, buf, size);

       	osSemaphoreRelease(tcpHandler->rxIrqSemaphoreHandle);
    }
    osSemaphoreRelease(tcpHandler->rxSemaphoreHandle);


exit:
	return result;
}

static ssize_t dummy_cookie_write(void *cookie, const char *buf, unsigned int size)
{
  struct StreamTcpHandler *tcpHandler = (struct StreamTcpHandler *)cookie;

  if (osSemaphoreWait(tcpHandler->txSemaphoreHandle, 10000) != osOK)
  {
    size = 0;
	goto exit;
  }


  tcp_write(tcpHandler->my_tcp, buf, size, 0);
  tcp_output(tcpHandler->my_tcp);

exit:
    return size;
}

static err_t tcpRecHandler(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	return ERR_OK;
}


