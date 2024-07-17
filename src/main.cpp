//#include "main.hpp"
#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

#include <stmcpp/register.hpp>
#include <stmcpp/units.hpp>
#include <stmcpp/clock.hpp>
#include <stmcpp/gpio.hpp>
#include <stmcpp/usart.hpp>
#include <stmcpp/spi.hpp>
#include <stmcpp/dma.hpp>
#include <stmcpp/bdma.hpp>
#include <stmcpp/dmamux.hpp>

#include "clock.hpp"

using namespace stmcpp::units;


stmcpp::gpio::pin<stmcpp::gpio::port::porte, 10> led0(stmcpp::gpio::mode::output);
stmcpp::gpio::pin<stmcpp::gpio::port::porte, 12> led1(stmcpp::gpio::mode::output);
stmcpp::gpio::pin<stmcpp::gpio::port::porte, 14> led2(stmcpp::gpio::mode::output);
stmcpp::gpio::pin<stmcpp::gpio::port::porte, 15> led3(stmcpp::gpio::mode::output);


extern "C" void SystemInit(void){
	// Enable the FPU if needed
	#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
		stmcpp::reg::set(std::ref(SCB->CPACR), (3UL << 20U)|(3UL << 22U));
    #endif

	// Initialize the system clock
	clock::init();

	// Enable the necessary peripheral clocks
	stmcpp::clock::enablePeripherals(
		stmcpp::clock::peripheral::gpioc,
		stmcpp::clock::peripheral::gpioe
	);
}

extern "C" int main(void){
	// Setup the systick timer to count with resolution of 1ms
	stmcpp::clock::systick::init(1_ms);

	while(1){
		stmcpp::clock::systick::waitBlocking(100_ms);
		led0.toggle();
	}
	
}

// Increment the systick timer
extern "C" void SysTick_Handler(){
    stmcpp::clock::systick::increment();
}

extern "C" void NMI_Handler(void) {
	// If the interrupt was triggered by HSE clock 
	if (stmcpp::reg::read(std::ref(RCC->CIFR), RCC_CIFR_HSECSSF)) {

		// Deal with it later, just loop endlessly for now
		while (true) {;}
	}
}

extern "C" int _write(int file, char* ptr, int len){
	// Implement for printf redirection
	return 0;
}

extern "C" void HardFault_Handler(void){
	//Ooops, hard fault! Disable interrupts
	__disable_irq();

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

