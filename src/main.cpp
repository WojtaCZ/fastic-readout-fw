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
#include <stmcpp/adc.hpp>
#include <stmcpp/dac.hpp>
#include <string>

#include <stmcpp/i2c.hpp>

#include "git.hpp"

#include "clock.hpp"
#include "si5340.hpp"
#include "ad9510.hpp"
#include "usb.hpp"
#include "fastic.hpp"

#include <tinyusb/src/device/usbd.h>



using namespace stmcpp::units;



//stmcpp::gpio::pin<stmcpp::gpio::port::porte, 10> led0(stmcpp::gpio::mode::output);
//stmcpp::gpio::pin<stmcpp::gpio::port::porte, 12> led1(stmcpp::gpio::mode::output);
//stmcpp::gpio::pin<stmcpp::gpio::port::porte, 14> led2(stmcpp::gpio::mode::output);

stmcpp::gpio::pin<stmcpp::gpio::port::porta, 0> usart4_tx(stmcpp::gpio::mode::af8);
stmcpp::usart::uart<stmcpp::usart::peripheral::uart4> usart4(4_MHz, stmcpp::usart::divider::noDivide, 115200_Bd);

extern "C" void SystemInit(void){
	// Enable the FPU if needed
	#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
		stmcpp::reg::set(std::ref(SCB->CPACR), (3UL << 20U) | (3UL << 22U));
    #endif

	// Initialize the system clock
	clock::init();

	// Configure the MPU to not cache D2 RAM - this is needed for the DMA buffers
	// We are configuring region 0
	stmcpp::reg::write(std::ref(MPU->RNR), 0);
	
	// Make sure the region is disabled
	stmcpp::reg::clear(std::ref(MPU->RASR), MPU_RASR_ENABLE_Msk);

	// Set the region base address - RAM D2
	stmcpp::reg::write(std::ref(MPU->RBAR), 0x30000000); 

	// Configure the region flags
	stmcpp::reg::write(std::ref(MPU->RASR),
		(0b0 		<< MPU_RASR_XN_Pos) |	// Instruction fetch enabled
		(0b011 		<< MPU_RASR_AP_Pos) |	// RW access
		(0b001 		<< MPU_RASR_TEX_Pos) |	// normaal, non shareable, non cacheable
		(0b0 		<< MPU_RASR_S_Pos) |
		(0b0 		<< MPU_RASR_C_Pos) |
		(0b0 		<< MPU_RASR_B_Pos) |
		(0b0 		<< MPU_RASR_SRD_Pos) |	// Sub region enabled
		(17 		<< MPU_RASR_SIZE_Pos) |	// Region size = 17 -> 2^(17+1) = 256KiB (this is smaller than the actual D2 size, but it's enough for all the buffers)
		(0b1 		<< MPU_RASR_ENABLE_Pos) // Enable the region
	);	

	// Enable the MPU
	stmcpp::reg::write(std::ref(MPU->CTRL), 
		(0b1 << MPU_CTRL_PRIVDEFENA_Pos) |
		(0b0 << MPU_CTRL_HFNMIENA_Pos) |
		(0b1 << MPU_CTRL_ENABLE_Pos) 
	);

	__ASM volatile("dsb");
	__ASM volatile("isb");

	


	// Enable the necessary peripheral clocks
	stmcpp::clock::enablePeripherals(
		stmcpp::clock::peripheral::gpioa,
		stmcpp::clock::peripheral::gpiob,
		stmcpp::clock::peripheral::gpioc,
        stmcpp::clock::peripheral::gpiod,
		stmcpp::clock::peripheral::gpioe,
        stmcpp::clock::peripheral::i2c1,
		stmcpp::clock::peripheral::i2c3,
		stmcpp::clock::peripheral::i2c4,
		stmcpp::clock::peripheral::uart4,
		stmcpp::clock::peripheral::tim1,
		stmcpp::clock::peripheral::tim15,
		// SPI and DMA used for aurora stream reception
		stmcpp::clock::peripheral::spi1,
		stmcpp::clock::peripheral::dma1,
		// USB clocks
		stmcpp::clock::peripheral::usb1otg,
		stmcpp::clock::peripheral::usb1ulpi
	);
}



extern "C" int main(void){
	// Enable the systick to run at 1ms
	stmcpp::clock::systick::enable(480_MHz, 1_ms);

	//usart4.enableTx();
	//usart4.enable();

	//printf("Test \n\r");

	//usb::init();


	
	
	//si5340::init();
	//ad9510::init();
	//fastic::init();
	fastic::initInjectionChannels();


	
	while(1){
		//tud_task();
		//stmcpp::clock::systick::waitBlocking(100_ms);
		
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

	for(int i = 0; i < len; i++){
		usart4.transmit(ptr[i]);

		duration timestamp_ = stmcpp::clock::systick::getDuration();

		while (!usart4.getStatusFlag(stmcpp::usart::flag::txFree)) {
			if(stmcpp::clock::systick::getDuration() > (timestamp_ + 500_ms)) {
				stmcpp::error::globalFaultHandler(0,0);
			}
		}
	}

	// Implement for printf redirection
	return 0;
}
extern "C" void __wrap_printf(char *format) {
    _write(0, format, strlen(format));
}    

void stmcpp::error::globalFaultHandler(std::uint32_t hash, std::uint32_t code) {
	//There has been an error caused by the handler, try to figure out what happened
	switch (hash) {
		case stmcpp::error::moduleHash("stmcpp::clock"):
				{
				stmcpp::clock::error err = static_cast<stmcpp::clock::error>(code);
				__ASM volatile("bkpt");
				}
			break;

		case stmcpp::error::moduleHash("stmcpp::i2c"):
				{
				stmcpp::i2c::error err = static_cast<stmcpp::i2c::error>(code);
				__ASM volatile("bkpt");
				}
			break;

		case stmcpp::error::moduleHash("ad9510"):
				{
				ad9510::error err = static_cast<ad9510::error>(code);
				__ASM volatile("bkpt");
				}
			break;
		
		case stmcpp::error::moduleHash("si5340"):
				{
				si5340::error err = static_cast<si5340::error>(code);
				__ASM volatile("bkpt");
				}
			break;
		
		case stmcpp::error::moduleHash("usb"):
				{
				usb::error err = static_cast<usb::error>(code);
				__ASM volatile("bkpt");
				}
			break;
		
		default:
			__ASM volatile("bkpt");
			break;
	}
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

