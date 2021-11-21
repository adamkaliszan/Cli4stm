#ifndef INC_STREAM_TCP_H_
#define INC_STREAM_TCP_H_

#include <stdio.h>

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"

#include "lwip/ip_addr.h"

#define NO_OF_TCP_SERVER_TASKS 1


int startTcpServer(uint16_t portNo, osThreadId *tasks);
int acceptTcpConnection(FILE** streamIn, FILE** streamOut, int handlerIdx);


#endif /* INC_STREAM_TCP_H_ */
