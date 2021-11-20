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

cookie_io_functions_t dummy_tcp_cookie_funcs = {
  .read = dummy_cookie_read,
  .write = dummy_cookie_write,
  .seek = 0,
  .close = 0
};

static err_t tcpRecHandler(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);


static err_t acceptCallback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	struct StreamTcpHandler *tcpHandler = NULL;
    for (int i=0; i < NO_OF_TCP_STREAM_HANDLERS; i++)
    {
    	if (StreamTcpHandlers[i].my_tcp == NULL)
    	{
    		tcpHandler = &StreamTcpHandlers[i];
    		break;
    	}
    }
    if (tcpHandler == NULL)
    	return ERR_CLSD;

    tcpHandler->my_tcp = newpcb;
    tcp_arg(newpcb, tcpHandler);

    return ERR_OK;
}

void startTcpServer(uint16_t portNo)
{
	osSemaphoreDef_t tmp = {.controlblock = NULL};
    for (int i=0; i < NO_OF_TCP_STREAM_HANDLERS; i++)
    {
        StreamTcpHandlers[i].txBuffer             = NULL;
    	StreamTcpHandlers[i].txSemaphoreHandle    = osSemaphoreCreate(&tmp, 1);
    	StreamTcpHandlers[i].rxSemaphoreHandle    = osSemaphoreCreate(&tmp, 1);
    	StreamTcpHandlers[i].rxIrqSemaphoreHandle = osSemaphoreCreate(&tmp, 1);
    }

	struct tcp_pcb *sck = tcp_new();      // Step 1      Call tcp_new to create a pcb.
	tcp_arg(sck, StreamTcpHandlers);      // Step 2      Optionally call tcp_arg to associate an application-specific value with the pcb.
	tcp_bind(sck, NULL, portNo);          // Step 3      Call tcp_bind to specify the local IP address and port.
	tcp_listen(sck);                      // Step 4      Call tcp_listen or tcp_listen_with_backlog. (note: these functions will free the pcb given as an argument and return a smaller listener pcb (e.g. tpcb = tcp_listen(tpcb)))
    tcp_accept(sck, acceptCallback);      // Step 5      Call tcp_accept to specify the function to be called when a new connection arrives. Note that there is no possibility of a socket being accepted before specifying the callback, because this is all run on the tcpip_thread.
}

int acceptTcpConnection(FILE** streamIn, FILE** streamOut, int taskNo)
{
	*streamIn  = fopencookie(&StreamTcpHandlers[taskNo], "r", dummy_tcp_cookie_funcs);
	*streamOut = fopencookie(&StreamTcpHandlers[taskNo], "w", dummy_tcp_cookie_funcs);

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


