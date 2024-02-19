#include "main.hpp"
#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

#include "clock.hpp"
#include "timer.hpp"
#include "status.hpp"


extern "C" void SystemInit(void){
	//Initialize the clock
	if(clock::init() != 0) status::error::set();
	RCC->APB4ENR |= (1 << RCC_APB4ENR_SYSCFGEN_Pos);


	//Enable the compensation cell
	SYSCFG->CCCSR |= (1 << SYSCFG_CCCSR_EN_Pos);

	while(!(SYSCFG->CCCSR & SYSCFG_CCCSR_READY_Msk)){};

}

extern "C" int main(void){

	//status::init();

	timer::init();
	//status::process();

	while(1){
		__ASM("nop");
		//status::process();
	}
	
}

extern "C" void HardFault_Handler(void){
	//Ooops, hard fault!
	__ASM("bkpt");

	/*
	What now?
	Check the CFSR
		DIVBYZERO - division by zero
		UNALIGNED - access of unaligned word
		NOCP - disabled coprocessor (is the FPU enabled?)
		INVPC
		INVSTATE - can occur 99% only if writing hand assembly
		UNDEFINSTR - undefined instruction (corrupted stack, wrong code path)

	Check the BFSR
		BFARVALID - indicates that BFAR holds the address causing the fault
		LSPERR & STKERR - might occur in case of stack overflow
		UNSTKERR - fault while returning from except (corrupted stack)
		IMPRECISERR - was the MCU able to determine the exact fault location?
		PRECISERR - instruction executed befor exception caused the fault
	*/
	
}





