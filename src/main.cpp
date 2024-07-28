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
#include <string>

#include <stmcpp/i2c.hpp>

#include "git.hpp"

#include "clock.hpp"

using namespace stmcpp::units;


stmcpp::gpio::pin<stmcpp::gpio::port::porte, 10> led0(stmcpp::gpio::mode::output);
stmcpp::gpio::pin<stmcpp::gpio::port::porte, 12> led1(stmcpp::gpio::mode::output);
stmcpp::gpio::pin<stmcpp::gpio::port::porte, 14> led2(stmcpp::gpio::mode::output);
stmcpp::gpio::pin<stmcpp::gpio::port::porte, 15> led3(stmcpp::gpio::mode::output);

extern "C" void SystemInit(void){
	// Enable the FPU if needed
	#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
		stmcpp::reg::set(std::ref(SCB->CPACR), (3UL << 20U) | (3UL << 22U));
    #endif

	// Initialize the system clock
	clock::init();

	// Enable the necessary peripheral clocks
	stmcpp::clock::enablePeripherals(
		stmcpp::clock::peripheral::gpioc,
		stmcpp::clock::peripheral::gpioe,
		stmcpp::clock::peripheral::gpiob,
		stmcpp::clock::peripheral::i2c1
	);
}



extern "C" int main(void){
	// Setup the systick timer to count with resolution of 1ms
	stmcpp::clock::systick::init(1_ms);

	// Configure the Si5340 i2c interface
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 6> si5340_scl(stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull, stmcpp::gpio::speed::high);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 7> si5340_sda(stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull, stmcpp::gpio::speed::high);
	stmcpp::i2c::i2c<stmcpp::i2c::peripheral::i2c1> si5340_i2c(0x0B, 4, 2, 0x0F, 0x13);

	si5340_i2c.enable();
	std::uint8_t data = si5340_i2c.read8bitAddress(0x0B, 0x77);

	if(data == 0x77) led1.set();

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

	std::uint32_t hfsr = stmcpp::reg::read(std::ref(SCB->HFSR));

	if (hfsr & SCB_HFSR_FORCED_Msk) {
		// The hardfault has been forced because of other fault (Mem, Usage or Bus), skip to the other breakpoints
		__ASM volatile("bkpt");
	} else if  (hfsr & SCB_HFSR_VECTTBL_Msk) {
		// The hardfault has been caused 
		__ASM volatile("bkpt");
	}

	std::uint32_t cfsr = stmcpp::reg::read(std::ref(SCB->CFSR));

	if (cfsr & SCB_CFSR_DIVBYZERO_Msk) {
		// You have tried to divide by zero
		__ASM volatile("bkpt");
	} else if (cfsr & SCB_CFSR_UNALIGNED_Msk) {
		// Unaligned access to memory
		__ASM volatile("bkpt");
	} else if (cfsr & SCB_CFSR_NOCP_Msk) {
		// You are trying to use a disabled coprocessor, is the FPU enabled?
		__ASM volatile("bkpt");
	} else if (cfsr & SCB_CFSR_UNDEFINSTR_Pos) {
		// Woah, undefined instruction, isn't the stack corrupted?
		__ASM volatile("bkpt");
	} else {
		/*
		Hmmm, something else went wrong... What now?
		Check the other CFSR bits
			INVPC
			INVSTATE - can occur 99% only if writing hand assembly
			BFARVALID - indicates that BFAR holds the address causing the fault
			LSPERR & STKERR - might occur in case of stack overflow
			UNSTKERR - fault while returning from except (corrupted stack)
			IMPRECISERR - was the MCU able to determine the exact fault location?
			PRECISERR - instruction executed befor exception caused the fault
		*/

		__ASM volatile("bkpt");
	}
}

