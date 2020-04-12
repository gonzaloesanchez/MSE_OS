/*==================[inclusions]=============================================*/

#include "main.h"

#include "board.h"

#include "MSE_OS_Core.h"


/*==================[macros and definitions]=================================*/

#define MILISEC		1000

#define PRIORIDAD_0		0
#define PRIORIDAD_1		1
#define PRIORIDAD_3		3

/*==================[Global data declaration]==============================*/

tarea g_sTarea1, g_sTarea2, g_sTarea3;	//prioridad 0
tarea g_sTarea4;						//prioridad 1
tarea g_sTarea5, g_sTarea6;				//prioridad 3

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
	uint32_t h = 0;
	uint32_t i = 0;
	while (1) {
		h++;
		i++;
	}
}

void tarea2(void)  {
	uint32_t j = 0;
	uint32_t k = 0;

	while (1) {
		j++;
		k++;
	}
}

void tarea3(void)  {
	uint32_t l = 0;
	uint32_t m = 0;

	while (1) {
		l++;
		m++;
	}
}

void tarea4(void)  {
	uint32_t l = 0;
	uint32_t m = 0;

	while (1) {
		l++;
		m++;
	}
}

void tarea5(void)  {
	uint32_t l = 0;
	uint32_t m = 0;

	while (1) {
		l++;
		m++;
	}
}

void tarea6(void)  {
	uint32_t l = 0;
	uint32_t m = 0;

	while (1) {
		l++;
		m++;
	}
}




/*============================================================================*/

int main(void)  {

	initHardware();

	os_InitTarea(tarea1, &g_sTarea1,PRIORIDAD_0);
	os_InitTarea(tarea2, &g_sTarea2,PRIORIDAD_0);
	os_InitTarea(tarea3, &g_sTarea3,PRIORIDAD_0);

	os_InitTarea(tarea5, &g_sTarea5,PRIORIDAD_3);
	os_InitTarea(tarea6, &g_sTarea6,PRIORIDAD_3);

	os_InitTarea(tarea4, &g_sTarea4,PRIORIDAD_1);

	os_Init();

	while (1) {
	}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/
