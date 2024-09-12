//#include "main.hpp"
#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

#include <stmcpp/register.hpp>
#include <stmcpp/units.hpp>
#include <stmcpp/clock.hpp>
#include <stmcpp/gpio.hpp>
#include <stmcpp/usart.hpp>
#include <stmcpp/dma.hpp>
#include <stmcpp/dmamux.hpp>

#include <string>


#include "git.hpp"

#include "clock.hpp"
#include "usb.hpp"

#include <tusb_config.h>
#include <tinyusb/src/tusb.h>
#include <tinyusb/src/device/usbd.h>

using namespace stmcpp::units;


std::uint32_t tmpReg;

// Here is some magic that should allow us to read / write ulpi directly
#define USBULPI_PHYCR     ((uint32_t)(0x40040000 + 0x034))
#define USBULPI_D07       ((uint32_t)0x000000FF)
#define USBULPI_New       ((uint32_t)0x02000000)
#define USBULPI_RW        ((uint32_t)0x00400000) 
#define USBULPI_S_BUSY    ((uint32_t)0x04000000) 
#define USBULPI_S_DONE    ((uint32_t)0x08000000)

#define USB_OTG_READ_REG32(reg)  (*(__IO uint32_t *)(reg))
#define USB_OTG_WRITE_REG32(reg,value) (*(__IO uint32_t *)(reg) = (value))

/**
  * @brief  Read CR value
  * @param  Addr the Address of the ULPI Register
  * @retval Returns value of PHY CR register
  */
extern "C" uint32_t USB_ULPI_Read(uint32_t Addr)
{ 
   __IO uint32_t val = 0;
   __IO uint32_t timeout = 1000; /* Can be tuned based on the Clock or etc... */
   
   USB_OTG_WRITE_REG32(USBULPI_PHYCR, USBULPI_New | (Addr << 16));
   val = USB_OTG_READ_REG32(USBULPI_PHYCR);
   while (((val & USBULPI_S_DONE) == 0) && (timeout--)) 
   { 
       val = USB_OTG_READ_REG32(USBULPI_PHYCR);
   } 
   val = USB_OTG_READ_REG32(USBULPI_PHYCR);  
   return  (val & 0x000000ff);
}

/**
  * @brief  Write CR value
  * @param  Addr the Address of the ULPI Register
  * @param  Data Data to write
  * @retval Returns value of PHY CR register
  */
extern "C" uint32_t USB_ULPI_Write(uint32_t Addr, uint32_t Data)   /* Parameter is the Address of the ULPI Register & Date to write */
{  
  __IO uint32_t val;
  __IO uint32_t timeout = 1000;   /* Can be tuned based on the Clock or etc... */
  
  USB_OTG_WRITE_REG32(USBULPI_PHYCR, USBULPI_New | USBULPI_RW | (Addr << 16) | (Data & 0x000000ff));
  val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  while (((val & USBULPI_S_DONE) == 0) && (timeout--)) 
  {
           val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  }
  
  val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  return 0; 
}

extern "C" void SystemInit(void){

	// Enable the LDO and wait for it to be ready
	PWR->CR3 |= PWR_CR3_LDOEN;
	while(!(PWR->CSR1 & PWR_CSR1_ACTVOSRDY_Msk)){;}

	//Enable the USB regulator and wait for it to be ready
	PWR->CR3 |= PWR_CR3_USBREGEN | PWR_CR3_USB33DEN;
	while(!(PWR->CR3 & PWR_CR3_USB33RDY_Msk)){;}

	//Enable sysconfig clock
	RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;

    //Set voltage scale to 1 and wait for it to be ready
	PWR->D3CR |= 0b11 << PWR_D3CR_VOS_Pos;
	while(!(PWR->D3CR & PWR_D3CR_VOSRDY_Msk)){;}

	//Enable overdrive and wait for the VOS to change to 0
	SYSCFG->PWRCR |= SYSCFG_PWRCR_ODEN;
	while(!(PWR->D3CR & PWR_D3CR_VOSRDY_Msk)){;}

	//Set the HSE on and wait for it to be ready
	RCC->CR |= RCC_CR_CSSHSEON | RCC_CR_HSEON;
	while(!(RCC->CR & RCC_CR_HSERDY_Msk)){;}

	//Set the HSI48 on and wait for it to be ready
	RCC->CR |= RCC_CR_HSI48ON;
	while(!(RCC->CR & RCC_CR_HSI48RDY_Msk)){;}

	//Set up the flash latency and wait for it to take effect
	constexpr std::uint32_t FLASH_ACR_CFG = (FLASH_ACR_LATENCY_4WS | (0b10 << FLASH_ACR_WRHIGHFREQ_Pos));
	FLASH->ACR = FLASH_ACR_CFG;
	while(FLASH->ACR != FLASH_ACR_CFG){;}

	//Setup the PLL1 for sysclock of 480MHz, source is HSE
	RCC->PLL1DIVR =	( (60 - 1) << RCC_PLL1DIVR_N1_Pos |
                	  (2 - 1) << RCC_PLL1DIVR_P1_Pos |
                      (4 - 1) << RCC_PLL1DIVR_Q1_Pos |
                      (2 - 1) << RCC_PLL1DIVR_R1_Pos 
                    );
	RCC->PLLCKSELR = (3 << RCC_PLLCKSELR_DIVM1_Pos) | RCC_PLLCKSELR_PLLSRC_HSE;
	RCC->PLLCFGR = (0b11 << RCC_PLLCFGR_PLL1RGE_Pos) | RCC_PLLCFGR_DIVP1EN;

	//Enable it and wait for it to lock
	RCC->CR |= RCC_CR_PLL1ON_Msk;	
	while(!(RCC->CR & RCC_CR_PLL1RDY_Msk)){;}

	//Setup the domain dividers
	RCC->D1CFGR = RCC_D1CFGR_D1PPRE_DIV1 | RCC_D1CFGR_D1PPRE_DIV2 | RCC_D1CFGR_HPRE_DIV2;
    RCC->D2CFGR = RCC_D2CFGR_D2PPRE1_DIV2 | RCC_D2CFGR_D2PPRE2_DIV2;
	RCC->D3CFGR = RCC_D3CFGR_D3PPRE_DIV2;

	//Switch to PLL1 and wait for lock
	RCC->CFGR |= RCC_CFGR_SW_PLL1;
	while((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL1){;}

	// Enable CSI (for clock compensation)
	RCC->CR |= RCC_CR_CSION;
    while(!(RCC->CR & RCC_CR_CSION_Msk)){;}

	// Enable clock compensation cell and wait for it to be ready
	SYSCFG->CCCSR |= SYSCFG_CCCSR_EN;
	while(!(SYSCFG->CCCSR & SYSCFG_CCCSR_READY_Msk)){;}

	// Select HSI48 as clock for the usb peripheral and check if it has been selected
    RCC->D2CCIP2R |= (0b11 << RCC_D2CCIP2R_USBSEL_Pos);
	while((RCC->D2CCIP2R & RCC_D2CCIP2R_USBSEL_Msk) != (0b11 << RCC_D2CCIP2R_USBSEL_Pos)){;}

	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN | RCC_AHB4ENR_GPIOEEN;
	RCC->AHB1ENR |= RCC_AHB1ENR_USB1OTGHSEN | RCC_AHB1ENR_USB1OTGHSULPIEN;

}

char * str;

extern "C" int main(void){
	// Setup the systick timer to count with resolution of 1ms
	stmcpp::clock::systick::init(1_ms);

	stmcpp::gpio::pin<stmcpp::gpio::port::porta, 2>  ulpi_rst (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain);
	// Control interface pins
	stmcpp::gpio::pin<stmcpp::gpio::port::porte, 7> refsel0 (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 2> refsel1 (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
	stmcpp::gpio::pin<stmcpp::gpio::port::portc, 5> refsel2 (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
	// We need to generate 12MHz on this pin
	//stmcpp::gpio::pin<stmcpp::gpio::port::porte, 9> refclk (stmcpp::gpio::mode::af1, stmcpp::gpio::otype::pushPull, stmcpp::gpio::speed::veryHigh);

	stmcpp::gpio::pin<stmcpp::gpio::port::porta, 3>  ulpi_d0 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::veryHigh);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 0>  ulpi_d1 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::veryHigh);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 1>  ulpi_d2 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::veryHigh);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 10> ulpi_d3 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::veryHigh);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 11> ulpi_d4 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::veryHigh);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 12> ulpi_d5 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::veryHigh);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 13> ulpi_d6 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::veryHigh);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 5>  ulpi_d7 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::veryHigh);

	stmcpp::gpio::pin<stmcpp::gpio::port::portc, 0>  ulpi_stp (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::veryHigh);
	stmcpp::gpio::pin<stmcpp::gpio::port::portc, 2>  ulpi_dir (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::veryHigh);
	stmcpp::gpio::pin<stmcpp::gpio::port::portc, 3>  ulpi_nxt (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::veryHigh);
	stmcpp::gpio::pin<stmcpp::gpio::port::porta, 5>  ulpi_clk (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::veryHigh);

	stmcpp::reg::clear(std::ref(SYSCFG->PMCR), SYSCFG_PMCR_PC2SO); 
    stmcpp::reg::clear(std::ref(SYSCFG->PMCR), SYSCFG_PMCR_PC3SO); 

	// Set up the clock input to be 12MHz
	ulpi_rst.clear();
	refsel0.clear();
	refsel1.set(); 
	refsel2.clear();
	stmcpp::clock::systick::waitBlocking(1_ms);
	ulpi_rst.set();
	stmcpp::clock::systick::waitBlocking(10_ms);


	// Perform the core init according to the datasheet
	// Unmask global interrupt and indicate that periodic fifo is empty
	stmcpp::reg::set(std::ref(USB1_OTG_HS->GAHBCFG), USB_OTG_GAHBCFG_GINT | USB_OTG_GAHBCFG_PTXFELVL);
	// Set RX FIFO not empty flag
	stmcpp::reg::set(std::ref(USB1_OTG_HS->GINTSTS), USB_OTG_GINTSTS_RXFLVL);
	
	// Configure the basics
	stmcpp::reg::write(std::ref(USB1_OTG_HS->GUSBCFG),  (0 << USB_OTG_GUSBCFG_PHYSEL_Pos)	  |
														(0 << USB_OTG_GUSBCFG_ULPIFSLS_Pos)	  |
														(0 << USB_OTG_GUSBCFG_HNPCAP_Pos) |  // Not HNP capable
														(0 << USB_OTG_GUSBCFG_SRPCAP_Pos) |  // Not SRP capable
														(6 << USB_OTG_GUSBCFG_TRDT_Pos) |    // Turnaround time6
														(7 << USB_OTG_GUSBCFG_TOCAL_Pos));   // Timeout calibration

	// Unmask global and mode mismatch interrupt
	stmcpp::reg::set(std::ref(USB1_OTG_HS->GINTMSK), USB_OTG_GINTMSK_OTGINT | USB_OTG_GINTMSK_MMISM);

	// Force device mode 
	stmcpp::reg::set(std::ref(USB1_OTG_HS->GUSBCFG), USB_OTG_GUSBCFG_FDMOD/* | USB_OTG_GUSBCFG_ULPIIPD*/ | USB_OTG_GUSBCFG_ULPICSM | USB_OTG_GUSBCFG_ULPIAR);	

	// wait for AHB master IDLE
	while (!(USB1_OTG_HS->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL_Msk)) {}	

	ulpi_rst.clear();
	stmcpp::clock::systick::waitBlocking(1_ms);
	ulpi_rst.set();
	stmcpp::clock::systick::waitBlocking(10_ms);



	USB_ULPI_Write(0x16, 0xAA);
	tmpReg = USB_ULPI_Read(0x16);

	for(int i = 0; i < 10; i++){
		tmpReg = USB_ULPI_Read(i);
		//sprintf(str, "Add: %02x   Val: %02lx\n\r", i, reg);
		//printf(str);
	}

	while(1){
		 //tud_task(); // tinyusb device task
		stmcpp::clock::systick::waitBlocking(100_ms);
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
/*
	for(int i = 0; i < len; i++){
		usart.transmit(ptr[i]);

		duration timestamp_ = stmcpp::clock::systick::getDuration();

		while (!usart.getStatusFlag(stmcpp::usart::flag::txFree)) {
			if(stmcpp::clock::systick::getDuration() > (timestamp_ + 500_ms)) {
				stmcpp::error::globalFaultHandler(0,0);
			}
		}
	}*/

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

		default:
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

