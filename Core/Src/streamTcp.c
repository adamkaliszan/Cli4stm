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
	struct tcp_pcb* my_tcp;
	osThreadId      task;

/// Receiver
	struct pbuf*    recBuffer;
	uint16_t        recBufRdPos;
/// Transmitter
	uint16_t        sentSize;
};


static ssize_t dummy_cookie_writeTcp(void *cookie, const char *buf, unsigned int remainingSize);
static ssize_t dummy_cookie_read(void *cookie, char *buf, unsigned int remainingSize);
static int dummy_cookie_closeTcp(void *cookie);

static err_t receivedCallback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t acceptCallback(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t sentCallback(void *arg, struct tcp_pcb *tpcb, u16_t len);

static struct StreamTcpHandler StreamTcpHandlers[NO_OF_TCP_SERVER_TASKS];


cookie_io_functions_t dummy_tcp_cookie_funcs = {
  .read = dummy_cookie_read,
  .write = dummy_cookie_writeTcp,
  .seek = 0,
  .close = dummy_cookie_closeTcp
};

static err_t acceptCallback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	(void) arg;
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

    return err;
}

static err_t receivedCallback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	struct StreamTcpHandler *tcpHandler = (struct StreamTcpHandler *) arg;
	if (p == NULL)
	{
		osSignalSet(tcpHandler->task, SIG_DISCONNECTED);
	}
	else
	{
		tcpHandler->recBuffer = p;
		osSignalSet(tcpHandler->task, SIG_DATA_IRQ);
	}
	return err;
}

static err_t sentCallback(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	struct StreamTcpHandler *tcpHandler = (struct StreamTcpHandler *) arg;
	tcpHandler->sentSize = len;
	osSignalSet(tcpHandler->task, SIG_TX_IRQ); //TODO sprawdzi??, czy wszystko zosta??o wys??ane
	return ERR_OK;
}

int startTcpServer(uint16_t portNo, osThreadId *tasks)
{
    for (int i=0; i < NO_OF_TCP_SERVER_TASKS; i++)
    {
        StreamTcpHandlers[i].my_tcp      = NULL;
        StreamTcpHandlers[i].task        = tasks[i];

        StreamTcpHandlers[i].recBuffer   = NULL;
        StreamTcpHandlers[i].recBufRdPos = 0;

        StreamTcpHandlers[i].sentSize    = 0;
    }

	struct tcp_pcb *sck = tcp_new();              // Step 1      Call tcp_new to create a pcb.
	//tcp_arg(sck, StreamTcpHandlers);            // Step 2      Optionally call tcp_arg to associate an application-specific value with the pcb.
	tcp_bind(sck, NULL, portNo);                  // Step 3      Call tcp_bind to specify the local IP address and port.
	struct tcp_pcb *new_tcp = tcp_listen(sck);    // Step 4      Call tcp_listen or tcp_listen_with_backlog. (note: these functions will free the pcb given as an argument and return a smaller listener pcb (e.g. tpcb = tcp_listen(tpcb)))
	tcp_accept(new_tcp, acceptCallback);          // Step 5      Call tcp_accept to specify the function to be called when a new connection arrives. Note that there is no possibility of a socket being accepted before specifying the callback, because this is all run on the tcpip_thread.

	return 0;
}

int acceptTcpConnection(FILE** streamIn, FILE** streamOut, int handlerIdx)
{
	if (handlerIdx < 0)
		return -1;
	if (handlerIdx >= NO_OF_TCP_SERVER_TASKS)
		return -2;

	struct StreamTcpHandler *handler = StreamTcpHandlers + handlerIdx;
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
			if (tcpHandler->recBuffer == NULL)
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

static ssize_t dummy_cookie_writeTcp(void *cookie, const char *buf, unsigned int size)
{
	struct StreamTcpHandler *tcpHandler = (struct StreamTcpHandler *)cookie;

	if (tcpHandler->my_tcp == NULL)
		return 0;

	err_t tmpRes;
	while (size > 0)
	{
		tcpHandler->sentSize = 0;
		tmpRes = tcp_write(tcpHandler->my_tcp, buf, size, TCP_WRITE_FLAG_COPY);
		tmpRes = tcp_output(tcpHandler->my_tcp);
		osSignalWait(SIG_TX_IRQ, osWaitForever);

		if (tmpRes == ERR_OK)
			break;
		size-= size;//TODO w????czy?? obs??ug?? notyfikacji... tcpHandler->sentSize;
	}
    return size;
}

static ssize_t dummy_cookie_read(void *cookie, char *buf, unsigned int size)
{
	struct StreamTcpHandler *tcpHandler = (struct StreamTcpHandler *)cookie;

	ssize_t result = 0;
	osEvent ret;
	while (tcpHandler->my_tcp == NULL)
	{
		ret = osSignalWait(SIG_DATA_CON, osWaitForever);
	}

    if (tcpHandler->recBuffer != NULL)
    {
    	result = procTcpBuffer(tcpHandler, buf, size);
    }
    else
    {
       	//tcp_recv(tcpHandler->my_tcp, receivedCallback);
       	ret = osSignalWait(SIG_DATA_IRQ | SIG_DISCONNECTED, osWaitForever);
       	if (ret.value.signals == SIG_DISCONNECTED)
       		return 0;

       	result = procTcpBuffer(tcpHandler, buf, size);
    }

	return result;
}

static int dummy_cookie_closeTcp(void *cookie)
{
	struct StreamTcpHandler *tcpHandler = (struct StreamTcpHandler *)cookie;
	if (tcpHandler->my_tcp != NULL)
	{
		;//TODO check it it is required tcp_free(tcpHandler->my_tcp);
	}
	tcpHandler->my_tcp = NULL;
	return 0;
}
