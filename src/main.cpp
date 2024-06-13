//#include "main.hpp"
#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

#include "clock.hpp"
#include "register.hpp"
//#include "dma.hpp"
//#include "bdma.hpp"
#include "gpio.hpp"
//#include "dmamux.hpp"
//#include "spi.hpp"
//#include "i2c.hpp"

//uint8_t databuff[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

//uint32_t d1 = SCB->CFSR;
//uint32_t d2 = SCB->BFSR;




extern "C" void SystemInit(void){
	//Initialize the clock
	clock::init();

	RCC->APB4ENR |= (1 << RCC_APB4ENR_SYSCFGEN_Pos);

	//reg::write<RCC_BASE + offsetof(RCC_TypeDef, APB4ENR)>(1 << RCC_APB4ENR_SYSCFGEN_Pos);


	//Enable the compensation cell
	SYSCFG->CCCSR |= (1 << SYSCFG_CCCSR_EN_Pos);

	while(!(SYSCFG->CCCSR & SYSCFG_CCCSR_READY_Msk)){};

	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;


}


gpio::pin<gpio::port::portb, 14> ledRed(gpio::mode::output);
gpio::pin<gpio::port::porta, 7> input(gpio::mode::input, gpio::otype::opendrain, gpio::pull::pullup);


extern "C" int main(void){


	
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	input.enableInterrupt(gpio::interrupt::edge::falling);



	while(1){
		for (int i = 0; i < 320000; i++){
			__ASM("nop");
		}

		//SPI1->TXDR = 0xAB;
		
		
	}
	
}

extern "C" void EXTI9_5_IRQHandler(){
		__ASM("nop");
		input.clearInterruptFlag();

		
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
