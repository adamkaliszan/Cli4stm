/*
 * cliTask.h
 *
 *  Created on: Apr 5, 2021
 *      Author: adam
 */

#ifndef SRC_CLITASK_H_
#define SRC_CLITASK_H_

#include "FreeRTOS.h"


void CliReceiverInit();

void CliSend(uint8_t *data, int len);


void StartCliTask(void const * argument);


#endif /* SRC_CLITASK_H_ */
