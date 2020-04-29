/*
 * g_OS_IRQ.h
 *
 *  Created on: 29 abr. 2018
 *      Author: gonza
 */

#ifndef MSE_OS_INC_MSE_OS_IRQ_H_
#define MSE_OS_INC_MSE_OS_IRQ_H_


#include "MSE_OS_Core.h"
#include "MSE_OS_API.h"
#include "board.h"
#include "cmsis_43xx.h"

#define CANT_IRQ	53

extern osControl g_sControl_OS;

bool os_InstalarIRQ(LPC43XX_IRQn_Type irq, void* usr_isr);
bool os_RemoverIRQ(LPC43XX_IRQn_Type irq);


#endif /* MSE_OS_INC_MSE_OS_IRQ_H_ */
