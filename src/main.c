/*==================[inclusions]=============================================*/

#include "main.h"

#include "board.h"


/*==================[macros and definitions]=================================*/

#define MILISEC		1000

/*==================[Global data declaration]==============================*/

uint32_t sp_antes, sp_durante, sp_despues;
uint32_t stackFrame[8];


/*==================[internal functions declaration]=========================*/

/** @brief hardware initialization function
 *	@return none
 */
static void initHardware(void);

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

static void initHardware(void)  {
	Board_Init();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / MILISEC);		//systick 1ms
}

/*==================[external functions definition]==========================*/

int main(void)  {

	initHardware();

	while (1) {
		asm ("mrs %[sp_antes], MSP" : [sp_antes] "=r" (sp_antes));
		__WFI();
		asm ("mrs %[sp_despues], MSP" : [sp_despues] "=r" (sp_despues));
	}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/
