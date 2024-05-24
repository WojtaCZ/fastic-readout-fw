#include "main.hpp"
#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

#include "clock.hpp"
#include "dma.hpp"
#include "bdma.hpp"
#include "gpio.hpp"
#include "dmamux.hpp"



extern "C" void SystemInit(void){
	//Initialize the clock
	if(clock::init() != 0) {
		__ASM("nop");
	};
	RCC->APB4ENR |= (1 << RCC_APB4ENR_SYSCFGEN_Pos);

	//reg::write<RCC_BASE + offsetof(RCC_TypeDef, APB4ENR)>(1 << RCC_APB4ENR_SYSCFGEN_Pos);


	//Enable the compensation cell
	SYSCFG->CCCSR |= (1 << SYSCFG_CCCSR_EN_Pos);

	while(!(SYSCFG->CCCSR & SYSCFG_CCCSR_READY_Msk)){};

}

extern "C" int main(void){
	
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;


	gpio::gpio<gpio::port::porte, 1, gpio::mode::pushpull, gpio::speed::low, gpio::pull::nopull> ledYellow;
	gpio::gpio<gpio::port::portb, 14, gpio::mode::pushpull, gpio::speed::low, gpio::pull::nopull> ledRed;
	
	dma::dma<dma::peripheral::dma1, dma::stream::stream0, dma::mode::periph2mem, dma::datasize::word, true, 0x000000, dma::datasize::word, false, static_cast<uint32_t>(SPI1_BASE) + offsetof(SPI_TypeDef, RXDR), 10> spi1dma;

	while(1){
		for (int i = 0; i < 3200000; i++){
			__ASM("nop");
		}
		ledYellow.toggle();
		ledRed.toggle();
		
		
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
