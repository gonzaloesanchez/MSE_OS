	/*
		.syntax unified

		Two slightly different syntaxes are support for ARM and THUMB instructions.
		The default, divided, uses the old style where ARM and THUMB instructions had their own,
		separate syntaxes. The new, unified syntax, which can be selected via the
		.syntax directive
	*/

	.syntax unified
	.global SysTick_Handler




	/*
		Se cambia a la seccion .data, donde se almacenan las variables en RAM
		Para ver data types en assembler
			--> http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0175m/Cbhifdhe.html
	*/
	.data
		estado: .word 0					//Variable necesaria para seguimiento de siguiente tarea;
		.extern sp_tarea1,sp_tarea2		//Variables donde se guardan los stack pointers


	/*
		Se cambia a la seccion .text, donde se almacena el programa en flash
	*/
	.text

	/*
		Indicamos que la proxima funcion debe ser tratada como codigo thumb al ser compilada
		Ver documentacion al respecto para mas detalles
	*/
	.thumb_func



SysTick_Handler:
	ldr r0, =estado		// r0 = &state
	ldr r1, [r0]		//r1 = *r0
	tbb [pc,r1]
tabla:							//construccion de un switch-case en asm
	.byte (case0-tabla)/2
	.byte (case1-tabla)/2
	.byte (case2-tabla)/2
	.byte (case_end-tabla)/2



/****** Si es el primer ingreso (estado == 0), debemos cargar en MSP el contenido de sp_tarea1 ***
		NOTA: Observar como es descartado el valor inicial de MSP (se sobreescribe)
		NOTA: Al descartar el valor inicial de MSP y a su vez no entrar mas en este caso
				no es necesario guardar los registros.											*/
case0:
	mov r1,1
	str r1,[r0]				//estado = 1

	ldr r0,=sp_tarea1		//R0 = &sp_tarea1
	ldr r1,[r0]				//R1 = *R0
	msr msp,r1				//MSP = R1 ==> MSP = sp_tarea1


	pop {R4-R11}
	/**
	*	el MSP esta apuntando al sp1, y el init task carga una pila de 16 registros
	*   debemos hacer el pop de los 8 que no se hacen automaticos para que el sp1 quede
	*	alineado a los que hacen unstacking de forma automatica. Los registros R4-R11 en
	*	este punto estan en realidad cargados con basura (datos no inicializados)
	*
	*	El primer llamado a la tarea1 solamente tiene este comportamiento, dado que
	*	estamos haciendo pop de valores de Stack no inicializados.
	*	Luego en sucesivas interrupciones y por lo tanto cambios de contexto, estos valores
	*	tendran sentido. Los valores R4-R11 al momento de cambiar el contexto a tarea2 para
	*	el primer ingreso a esa tarea ya contienen valores validos, modificados por tarea1,
	*	que deben ser guardados en el stack
	*/


	//break;
	b case_end


/****** Si se expropio el CPU a la tarea1 (estado == 1), debemos cargar en MSP el contenido de sp_tarea2 ****/
case1:
	mov r1,2
	str r1,[r0]		//estado = 2

	push {r4-r11}			//guardamos en el stack los registros restantes

	ldr r0,=sp_tarea1		//R0 = &sp_tarea1
	mrs r1,msp				//R1 = MSP
	str r1,[r0]				//*R0 = R1 ==> sp_tarea1 = MSP

	ldr r0,=sp_tarea2		//R0 = &sp_tarea2
	ldr r1,[r0]				//R1 = *R0
	msr msp,r1				//MSP = R1 ==> MSP = sp_tarea1

	pop {r4-r11}			//devolvemos a los registros los valores guardados en el stack

	//break;
	b case_end


/****** Si se expropio el CPU a la tarea2 (estado == 2), debemos cargar en MSP el contenido de sp_tarea1 ****/
case2:
	mov r1,1
	str r1,[r0]		//estado = 1

	push {r4-r11}			//guardamos en el stack los registros restantes

	ldr r0,=sp_tarea2		//R0 = &sp_tarea2
	mrs r1,msp				//R1 = MSP
	str r1,[r0]				//*R0 = R1 ==> sp_tarea2 = MSP

	ldr r0,=sp_tarea1		//R0 = &sp_tarea1
	ldr r1,[r0]				//R1 = *R0
	msr msp,r1				//MSP = R1 ==> MSP = sp_tarea1

	pop {r4-r11}			//devolvemos a los registros los valores guardados en el stack

case_end:
	bx lr			//Branch indirecto, se carga en PC lo que hay el LR (EXEC_RETURN)
