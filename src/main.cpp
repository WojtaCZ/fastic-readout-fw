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
#include <vector>

#include <stmcpp/i2c.hpp>

#include "git.hpp"

#include "clock.hpp"
#include "si5340.hpp"
#include "ad9510.hpp"




using namespace stmcpp::units;

float fasitVoltage, temperature = 0;

// Resaling needs to be done because the values in TSCAL are measured with VREF = 3.3V
float tscal1 = (double)(*(uint16_t*)(0x1ff1e820)) / (3.3 / 1.8);
float tscal2 = (double)(*(uint16_t*)(0x1ff1e840)) / (3.3 / 1.8);


extern "C" void SystemInit(void){
	// Enable the FPU if needed
	#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
		stmcpp::reg::set(std::ref(SCB->CPACR), (3UL << 20U) | (3UL << 22U));
    #endif

	// Initialize the system clock
	clock::init();

	// Select HSE as PER clock
	stmcpp::reg::change(std::ref(RCC->D1CCIPR), 0b11, 0b10, RCC_D1CCIPR_CKPERSEL_Pos);
	// Select PER as ADC clock
	stmcpp::reg::change(std::ref(RCC->D3CCIPR), 0b11, 0b10, RCC_D3CCIPR_ADCSEL_Pos);

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
		stmcpp::clock::peripheral::dma2,
		stmcpp::clock::peripheral::bdma,
		stmcpp::clock::peripheral::adc12,
		stmcpp::clock::peripheral::adc3,
		stmcpp::clock::peripheral::dac12,
		// USB clocks
		stmcpp::clock::peripheral::usb1otg,
		stmcpp::clock::peripheral::usb1ulpi
	);
}


stmcpp::gpio::pin<stmcpp::gpio::port::porta, 6> adc1_in3 (stmcpp::gpio::mode::analog);
stmcpp::adc::adc<stmcpp::adc::peripheral::adc1> adc1 (stmcpp::adc::resolution::sixteenBit);
static constexpr stmcpp::adc::channel adc1_ch3(3, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);
std::vector<stmcpp::adc::channel> adc1_sequence = {adc1_ch3};

stmcpp::gpio::pin<stmcpp::gpio::port::porta, 4> dac1_out1 (stmcpp::gpio::mode::analog);
stmcpp::dac::dac<stmcpp::dac::channel::ch1> dac1_ch1;

stmcpp::adc::adc<stmcpp::adc::peripheral::adc3> adc3 (stmcpp::adc::resolution::sixteenBit);
static constexpr stmcpp::adc::channel adc3_ch18(18, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);
std::vector<stmcpp::adc::channel> adc3_sequence = {adc3_ch18};

extern "C" int main(void){
	// Enable the systick to run at 1ms
	stmcpp::clock::systick::enable(480_MHz, 1_ms);

	//si5340::init();
	//ad9510::init();

	stmcpp::reg::set(std::ref(ADC12_COMMON->CCR), 0b1001, ADC_CCR_PRESC_Pos);
	stmcpp::reg::set(std::ref(ADC3_COMMON->CCR), 0b1001, ADC_CCR_PRESC_Pos);
	stmcpp::reg::set(std::ref(ADC3_COMMON->CCR), ADC_CCR_TSEN);

	adc1.calibrate(stmcpp::adc::calibration::singleEnded);
	adc1.setupRegularSequence(adc1_sequence);
	adc1.enableInterrupt(stmcpp::adc::interrupt::endOfConversion);
	NVIC_EnableIRQ(ADC_IRQn);

	adc3.calibrate(stmcpp::adc::calibration::singleEnded);
	adc3.setupRegularSequence(adc3_sequence);
	adc3.enableInterrupt(stmcpp::adc::interrupt::endOfConversion);
	NVIC_EnableIRQ(ADC3_IRQn);

	dac1_ch1.enable();

	adc1.enable();
	//adc3.enable();
	//adc3.startRegular();
	adc1.startRegular();

	while(1){
		stmcpp::clock::systick::waitBlocking(1000_ms);
		//adc1.startRegular();

	}
	
}

extern "C" void ADC1_2_IRQHandler(){
	stmcpp::reg::waitForBitSet(std::ref(ADC1->ISR), ADC_ISR_EOC);
	uint32_t data = ADC1->DR;
	fasitVoltage = (1.8/65538.0)*((float)data);
	adc1.clearInterruptFlag(stmcpp::adc::interrupt::endOfConversion);
	dac1_ch1.setValue(data >> 4);
	//dac1_ch1.trigger();
	NVIC_ClearPendingIRQ(ADC_IRQn);
	adc1.startRegular();

}

extern "C" void ADC3_IRQHandler(){
	stmcpp::reg::waitForBitSet(std::ref(ADC3->ISR), ADC_ISR_EOC);
	uint16_t data = ADC3->DR;
	temperature = ((110.0 - 30.0) / (double)(tscal2 - tscal1)) * (double)(data - tscal1) + 30.0;
	adc3.clearInterruptFlag(stmcpp::adc::interrupt::endOfConversion);
	NVIC_ClearPendingIRQ(ADC3_IRQn);
	adc3.startRegular();
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

