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

//#include "git.hpp"

#include "clock.hpp"
#include "si5340.hpp"
#include "ad9510.hpp"
#include "usb.hpp"
#include "fastic.hpp"
#include "readout.hpp"
#include "power.hpp"
#include "memory.hpp"
#include "analog.hpp"
#include "scheduler.hpp"
#include "communication.hpp"

#include <tinyusb/src/device/usbd.h>
#include <tinyusb/src/class/cdc/cdc_device.h>
#include <tinyusb/src/class/vendor/vendor_device.h>

stmcpp::gpio::pin<stmcpp::gpio::port::porti, 5> ledRed(stmcpp::gpio::mode::output);
stmcpp::gpio::pin<stmcpp::gpio::port::porti, 6> ledGreen(stmcpp::gpio::mode::output);
stmcpp::gpio::pin<stmcpp::gpio::port::porti, 7> ledBlue(stmcpp::gpio::mode::output);
stmcpp::gpio::pin<stmcpp::gpio::port::portd, 10> ledUSB(stmcpp::gpio::mode::output);
    

using namespace stmcpp::units;

void keepalive(){
	ledGreen.toggle();
}

void log(){
	printf("Voltages: FastIC1: %f, FastIC2: %f, Vbat: %f, Temperature: %f\n\r", analog::getFastIC1Voltage(), analog::getFastIC2Voltage(), analog::getVbatVoltage(), analog::getTemperature());
	

}

void log2(){

	//communication::sendStatus();
	//printf("HV: V: %f [V], I: %f [uA]\n\r", hv::getVoltage(), hv::getCurrent());
}

scheduler keepaliveScheduler = scheduler(200, &keepalive, scheduler::PERIODICAL | scheduler::ACTIVE);
scheduler logScheduler = scheduler(100, &log2, scheduler::PERIODICAL | scheduler::ACTIVE);

bool s = false;

extern "C" void SystemInit(void){
	// Enable the FPU if needed
	#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
		stmcpp::reg::set(std::ref(SCB->CPACR), (3UL << 20U) | (3UL << 22U));
    #endif

	// Initialize the system clock
	clock::init();

	// Disable caching in the D2 region where the DMA buffers are stored
	memory::disableCachingD2();

	// Disable caching in the D3 region where the BDMA buffers are stored
	memory::disableCachingD3();

	memory::enableMPU();

	// Enable the necessary peripheral clocks
	stmcpp::clock::enablePeripherals(
		stmcpp::clock::peripheral::gpioa,
		stmcpp::clock::peripheral::gpiob,
		stmcpp::clock::peripheral::gpioc,
        stmcpp::clock::peripheral::gpiod,
		stmcpp::clock::peripheral::gpioe,
		stmcpp::clock::peripheral::gpiof,
		stmcpp::clock::peripheral::gpiog,
		stmcpp::clock::peripheral::gpioh,
		stmcpp::clock::peripheral::gpioi,
		stmcpp::clock::peripheral::gpioj,
		stmcpp::clock::peripheral::gpiok,
        stmcpp::clock::peripheral::i2c1,
		stmcpp::clock::peripheral::i2c2,
		stmcpp::clock::peripheral::i2c3,
		stmcpp::clock::peripheral::i2c4,
		stmcpp::clock::peripheral::uart4,
		stmcpp::clock::peripheral::tim1,
		stmcpp::clock::peripheral::tim15,
		stmcpp::clock::peripheral::tim12,
		stmcpp::clock::peripheral::adc12,
		stmcpp::clock::peripheral::adc3,
		stmcpp::clock::peripheral::dac12,
		stmcpp::clock::peripheral::vrefbuf,
		// SPI and DMA used for aurora stream reception
		stmcpp::clock::peripheral::spi1,
		stmcpp::clock::peripheral::spi2,
		stmcpp::clock::peripheral::spi3,
		stmcpp::clock::peripheral::spi4,
		stmcpp::clock::peripheral::dma1,
		stmcpp::clock::peripheral::dma2,
		stmcpp::clock::peripheral::bdma,
		// USB clocks
		stmcpp::clock::peripheral::usb1otg,
		stmcpp::clock::peripheral::usb1ulpi
	);
}

float f = 20.0;
uint32_t i = (uint32_t)f;

extern "C" int main(void){
	// Enable the systick to run at 1ms
	stmcpp::clock::systick::enable(480_MHz, 1_ms);

	//setvbuf(stdout, NULL, _IONBF, 0);

	usb::init();

	//usart4.enableTx();
	//usart4.enable();

	
	si5340::init();
	fastic::init();
	analog::init();
	hv::init();
	//fastic::initInjectionChannels();

	ledRed.set();
	uint32_t aval;

	int t1, t2;
	
	while(1){
		tud_task();	
		communication::process();
		//cdc_task();
		
		keepaliveScheduler.dispatch();
		logScheduler.dispatch();
		//tud_vendor_n_write(, "Hello", 5);
		
		/*if(tud_vendor_mounted()){
			t1 = stmcpp::clock::systick::getTicks();
			tud_vendor_n_write(0, fastic1_buffers[0], 1024);
			t2 = stmcpp::clock::systick::getTicks();
		}*/
		
	}
	
}

// Increment the systick timer
extern "C" void SysTick_Handler(){
    stmcpp::clock::systick::increment();
	keepaliveScheduler.increment();
	logScheduler.increment();
}

extern "C" void NMI_Handler(void) {
	// If the interrupt was triggered by HSE clock 
	if (stmcpp::reg::read(std::ref(RCC->CIFR), RCC_CIFR_HSECSSF)) {

		// Deal with it later, just loop endlessly for now
		while (true) {;}
	}
}


extern "C" int _write(int file, char* ptr, int len){

	tud_cdc_write(ptr, len);
    tud_cdc_write_flush();

	/*for(int i = 0; i < len; i++){
		usart4.transmit(ptr[i]);

		duration timestamp_ = stmcpp::clock::systick::getDuration();

		while (!usart4.getStatusFlag(stmcpp::usart::flag::txFree)) {
			if(stmcpp::clock::systick::getDuration() > (timestamp_ + 500_ms)) {
				stmcpp::error::globalFaultHandler(0,0);
			}
		}
	}*/

	// Implement for printf redirection
	return 0;
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
		
		case stmcpp::error::moduleHash("stmcpp::adc"):
				{
				stmcpp::adc::error err = static_cast<stmcpp::adc::error>(code);
				__ASM volatile("bkpt");
				}
			break;

		case stmcpp::error::moduleHash("stmcpp::dac"):
				{
				stmcpp::dac::error err = static_cast<stmcpp::dac::error>(code);
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

