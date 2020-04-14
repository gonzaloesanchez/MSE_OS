/*==================[inclusions]=============================================*/

#include <stdlib.h>
#include "main.h"
#include "board.h"
#include "MSE_OS_Core.h"
#include "MSE_OS_API.h"
#include "sapi.h"


/*==================[macros and definitions]=================================*/

#define MILISEC		1000

#define PRIORIDAD_0		0
#define PRIORIDAD_1		1
#define PRIORIDAD_3		3

/*==================[Global data declaration]==============================*/

tarea g_sTarea1, g_sTarea2, g_sTarea3;	//prioridad 0
tarea g_sBotones;	//prioridad 3

osSemaforo semLed1, semLed2, semLed3;

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/** @brief hardware initialization function
 *	@return none
 */
static void initHardware(void)  {
	Board_Init();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / MILISEC);		//systick 1ms
}


/*==================[Definicion de tareas para el OS]==========================*/
void tarea1(void)  {
	uint32_t i;

	while (1) {
		i++;

		if (i%9 == 0)
			os_SemaforoTake(&semLed1);
		gpioWrite(LED1,true);
		os_Delay(rand() % 200 + 50);
		gpioWrite(LED1,false);
		os_Delay(rand() % 200 + 50);
	}
}

void tarea2(void)  {
	uint32_t j;

	while (1) {
		j++;

		if (j%9 == 0)
			os_SemaforoTake(&semLed2);
		gpioWrite(LED2,true);
		os_Delay(rand() % 100 + 50);
		gpioWrite(LED2,false);
		os_Delay(rand() % 100 + 50);
	}
}

void tarea3(void)  {
	uint32_t k = 0;

	while (1) {
		k++;
		os_SemaforoTake(&semLed3);
		gpioWrite(LED3,true);
		os_Delay(1000);
		gpioWrite(LED3,false);
	}
}

void botones(void)  {
	while(1)  {
		if(!gpioRead( TEC1 ))
			os_SemaforoGive(&semLed1);

		if(!gpioRead( TEC2 ))
			os_SemaforoGive(&semLed2);

		if(!gpioRead( TEC3 ))
			os_SemaforoGive(&semLed3);

		os_Delay(100);
	}
}


/*============================================================================*/

int main(void)  {

	initHardware();

	os_InitTarea(tarea1, &g_sTarea1,PRIORIDAD_0);
	os_InitTarea(tarea2, &g_sTarea2,PRIORIDAD_0);
	os_InitTarea(tarea3, &g_sTarea3,PRIORIDAD_0);
	os_InitTarea(botones, &g_sBotones,PRIORIDAD_3);

	os_SemaforoInit(&semLed1);
	os_SemaforoInit(&semLed2);
	os_SemaforoInit(&semLed3);

	os_Init();

	while (1) {
	}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/
