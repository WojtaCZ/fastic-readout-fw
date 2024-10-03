//#include "main.hpp"
#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

#include <stmcpp/register.hpp>
#include <stmcpp/units.hpp>
#include <stmcpp/clock.hpp>
#include <stmcpp/gpio.hpp>

#include <string>

#include "git.hpp"

using namespace stmcpp::units;

std::uint32_t devMode = 0, scratchRegister = 0;


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
   __IO uint32_t timeout = 10000000; /* Can be tuned based on the Clock or etc... */
   
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
  __IO uint32_t timeout = 10000000;   /* Can be tuned based on the Clock or etc... */
  
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

	//Enable the USB voltage detector
	PWR->CR3 |= PWR_CR3_USB33DEN;

	//Enable sysconfig clock
	RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;

    //Set voltage scale to 1 and wait for it to be ready
	PWR->D3CR |= 0b11 << PWR_D3CR_VOS_Pos;
	while(!(PWR->D3CR & PWR_D3CR_VOSRDY_Msk)){;}

	//Enable overdrive and wait for the VOS to change to 0
	SYSCFG->PWRCR |= SYSCFG_PWRCR_ODEN;
	while(!(PWR->D3CR & PWR_D3CR_VOSRDY_Msk)){;}

	//Enable HSE on and wait for it to be ready
	RCC->CR |= RCC_CR_CSSHSEON | RCC_CR_HSEON;
	while(!(RCC->CR & RCC_CR_HSERDY_Msk)){;}

	//Enable HSI48 on and wait for it to be ready
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

	// Enable the necessary GPIO and USB clocks
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN | RCC_AHB4ENR_GPIOEEN;
	RCC->AHB1ENR |= RCC_AHB1ENR_USB1OTGHSEN | RCC_AHB1ENR_USB1OTGHSULPIEN;

	__ASM volatile("dsb");

}

char * str;

extern "C" int main(void){
	// Setup the systick timer to count with resolution of 1ms
	stmcpp::clock::systick::init(1_ms);

	stmcpp::gpio::pin<stmcpp::gpio::port::porta, 2>  ulpi_rst (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);

	// The clock reference selection pins
	stmcpp::gpio::pin<stmcpp::gpio::port::porte, 7> refsel0 (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 2> refsel1 (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
	stmcpp::gpio::pin<stmcpp::gpio::port::portc, 5> refsel2 (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);

	// ULPI pins
	stmcpp::gpio::pin<stmcpp::gpio::port::porta, 3>  ulpi_d0 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 0>  ulpi_d1 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 1>  ulpi_d2 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 10> ulpi_d3 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 11> ulpi_d4 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 12> ulpi_d5 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 13> ulpi_d6 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
	stmcpp::gpio::pin<stmcpp::gpio::port::portb, 5>  ulpi_d7 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);

	stmcpp::gpio::pin<stmcpp::gpio::port::portc, 0>  ulpi_stp (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
	stmcpp::gpio::pin<stmcpp::gpio::port::portc, 2>  ulpi_dir (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
	stmcpp::gpio::pin<stmcpp::gpio::port::portc, 3>  ulpi_nxt (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
	stmcpp::gpio::pin<stmcpp::gpio::port::porta, 5>  ulpi_clk (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);

	// Make sure that the PC2 and PC3 analog switches are closed
	SYSCFG->PMCR &= ~SYSCFG_PMCR_PC2SO; 
    SYSCFG->PMCR &= ~SYSCFG_PMCR_PC3SO; 

	// Set up the reference clock to 24MHz (all refsel pins high)
	ulpi_rst.clear();
	refsel0.set();
	refsel1.set(); 
	refsel2.set();
	stmcpp::clock::systick::waitBlocking(20_ms);
	ulpi_rst.set();
	stmcpp::clock::systick::waitBlocking(100_ms);

	// Perform the core init according to the datasheet
	// Unmask global interrupt and indicate that periodic fifo is empty
	USB1_OTG_HS->GAHBCFG |= USB_OTG_GAHBCFG_GINT | USB_OTG_GAHBCFG_PTXFELVL;
	// Set RX FIFO not empty flag
	USB1_OTG_HS->GINTSTS |= USB_OTG_GINTSTS_RXFLVL;
	
	// Configure the basics
	USB1_OTG_HS->GUSBCFG |=    ((0 << USB_OTG_GUSBCFG_PHYSEL_Pos)	  |
								(1 << USB_OTG_GUSBCFG_ULPIAR_Pos)	  |
								(0 << USB_OTG_GUSBCFG_ULPIFSLS_Pos)	  |
								(0 << USB_OTG_GUSBCFG_ULPIEVBUSI_Pos)	  |
								(0 << USB_OTG_GUSBCFG_ULPIEVBUSD_Pos)	  |
								(0 << USB_OTG_GUSBCFG_HNPCAP_Pos) |  // Not HNP capable
								(0 << USB_OTG_GUSBCFG_SRPCAP_Pos) |  // Not SRP capable
								(9 << USB_OTG_GUSBCFG_TRDT_Pos) |    // Turnaround time
								(7 << USB_OTG_GUSBCFG_TOCAL_Pos));   // Timeout calibration

	// Unmask global and mode mismatch interrupt
	USB1_OTG_HS->GINTMSK |=  (USB_OTG_GINTMSK_OTGINT | USB_OTG_GINTMSK_MMISM);
	
	// Reset the core to clean interrupt flags
	while (!(USB1_OTG_HS->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL)) {}
	USB1_OTG_HS->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;
	while ((USB1_OTG_HS->GRSTCTL & USB_OTG_GRSTCTL_CSRST) == USB_OTG_GRSTCTL_CSRST) {}

	// Reset the ULPI interface (this seems to be needed, otherwise, there is no activity on the ULPI bus)
	// Also, this needs to be done after the core reset
	ulpi_rst.clear();
	stmcpp::clock::systick::waitBlocking(20_ms);
	ulpi_rst.set();
	stmcpp::clock::systick::waitBlocking(100_ms);


	while(1){
		scratchRegister = USB_ULPI_Read(0x00);
		if(scratchRegister == 0x24)	__ASM volatile("bkpt");
		stmcpp::clock::systick::waitBlocking(100_ms);
	}
	
}

// Increment the systick timer
extern "C" void SysTick_Handler(){
    stmcpp::clock::systick::increment();
}

extern "C" void HardFault_Handler(void){
	//Ooops, hard fault! Disable interrupts
	__disable_irq();
		__ASM volatile("bkpt");
}

void stmcpp::error::globalFaultHandler(std::uint32_t hash, std::uint32_t code) {
	__ASM volatile("bkpt");
}
