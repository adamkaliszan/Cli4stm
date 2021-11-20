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

#define SIG_DATA_CON 0x01
#define SIG_DATA_IRQ 0x02
#define SIG_DISCONNECTED 0x04
#define SIG_TX_IRQ 0x08

struct StreamTcpHandler
{
	struct tcp_pcb *my_tcp;

/// Receiver
	osThreadId    task;
//	SemaphoreId   rxSemaphoreHandle;
//	osSemaphoreId   rxIrqSemaphoreHandle;
	struct pbuf    *recBuffer;
	uint16_t        recBufRdPos;


/// Transmitter
//	osSemaphoreId   txSemaphoreHandle;
	uint16_t      sentSize;
};


static ssize_t dummy_cookie_write(void *cookie, const char *buf, unsigned int remainingSize);
static ssize_t dummy_cookie_read(void *cookie, char *buf, unsigned int remainingSize);

static err_t receivedCallback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t acceptCallback(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t sentCallback(void *arg, struct tcp_pcb *tpcb, u16_t len);


static struct StreamTcpHandler StreamTcpHandlers[NO_OF_TCP_SERVER_TASKS];


cookie_io_functions_t dummy_tcp_cookie_funcs = {
  .read = dummy_cookie_read,
  .write = dummy_cookie_write,
  .seek = 0,
  .close = 0
};

static err_t acceptCallback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	struct StreamTcpHandler *tcpHandler = NULL;
    for (int i=0; i < NO_OF_TCP_SERVER_TASKS; i++)
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
    tcp_recv(newpcb, receivedCallback);
    tcp_sent(newpcb, sentCallback);


    osSignalSet(tcpHandler->task, SIG_DATA_CON);

    return ERR_OK;
}

static err_t receivedCallback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	struct StreamTcpHandler *tcpHandler = (struct StreamTcpHandler *) arg;
	if (p == NULL)
		osSignalSet(tcpHandler->task, SIG_DISCONNECTED);
	else
		osSignalSet(tcpHandler->task, SIG_DATA_IRQ);
	return err;
}

static err_t sentCallback(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	struct StreamTcpHandler *tcpHandler = (struct StreamTcpHandler *) arg;
	tcpHandler->sentSize = len;
	osSignalSet(tcpHandler->task, SIG_TX_IRQ); //TODO sprawdzić, czy wszystko zostało wysłane
	return ERR_OK;
}

void startTcpServer(uint16_t portNo, osThreadId *tasks)
{
    for (int i=0; i < NO_OF_TCP_SERVER_TASKS; i++)
    {
        StreamTcpHandlers[i].recBuffer = NULL;
    	StreamTcpHandlers[i].task      = tasks[i];
    }

	struct tcp_pcb *sck = tcp_new();      // Step 1      Call tcp_new to create a pcb.
	tcp_arg(sck, StreamTcpHandlers);      // Step 2      Optionally call tcp_arg to associate an application-specific value with the pcb.
	tcp_bind(sck, NULL, portNo);          // Step 3      Call tcp_bind to specify the local IP address and port.
	tcp_listen(sck);                      // Step 4      Call tcp_listen or tcp_listen_with_backlog. (note: these functions will free the pcb given as an argument and return a smaller listener pcb (e.g. tpcb = tcp_listen(tpcb)))
    tcp_accept(sck, acceptCallback);      // Step 5      Call tcp_accept to specify the function to be called when a new connection arrives. Note that there is no possibility of a socket being accepted before specifying the callback, because this is all run on the tcpip_thread.
}

int acceptTcpConnection(FILE** streamIn, FILE** streamOut, void const *arg)
{
	struct StreamTcpHandler * const handler =  (struct StreamTcpHandler * const) arg;
	*streamIn  = fopencookie(handler, "r", dummy_tcp_cookie_funcs);
	*streamOut = fopencookie(handler, "w", dummy_tcp_cookie_funcs);

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

	while (tcpHandler->my_tcp == NULL)
	{
		osSignalWait(SIG_DATA_CON, osWaitForever);
	}

    if (tcpHandler->recBuffer != NULL)
    {
    	result = procTcpBuffer(tcpHandler, buf, size);
    }
    else
    {
       	//tcp_recv(tcpHandler->my_tcp, receivedCallback);
       	osEvent evResult = osSignalWait(SIG_DATA_IRQ | SIG_DISCONNECTED, osWaitForever);
       	if (evResult.value.signals == SIG_DISCONNECTED)
       	{
       		tcpHandler->my_tcp = NULL;
       		return 0;
       	}
       	result = procTcpBuffer(tcpHandler, buf, size);
    }

	return result;
}

static ssize_t dummy_cookie_write(void *cookie, const char *buf, unsigned int size)
{
	struct StreamTcpHandler *tcpHandler = (struct StreamTcpHandler *)cookie;

	while (tcpHandler->my_tcp == NULL)
	{
		osSignalWait(SIG_DATA_CON, osWaitForever);
	}


	while (size > 0)
	{
		tcpHandler->sentSize = 0;
		tcp_write(tcpHandler->my_tcp, buf, size, 0);
		tcp_output(tcpHandler->my_tcp);
		osSignalWait(SIG_TX_IRQ, osWaitForever);

		size-= tcpHandler->sentSize;
	}
    return size;
}
