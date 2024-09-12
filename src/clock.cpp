#include "clock.hpp"
#include <stmcpp/register.hpp>
#include <stmcpp/clock.hpp>

uint32_t SystemCoreClock = stmcpp::config::sysclock.toHertz();
uint32_t tmpReg;

void clock::init(){
	// Enable the LDO
	stmcpp::reg::set(std::ref(PWR->CR3), 1, PWR_CR3_LDOEN);
	// Wait for ready flag
	while(!stmcpp::reg::read(std::ref(PWR->CSR1), PWR_CSR1_ACTVOSRDY_Msk)){;}

    //Set voltage scale to 1
	stmcpp::reg::set(std::ref(PWR->D3CR), 0b11, PWR_D3CR_VOS_Pos);
	tmpReg = stmcpp::reg::read(std::ref(PWR->D3CR), PWR_D3CR_VOS_Msk);

	while(!stmcpp::reg::read(std::ref(PWR->D3CR), PWR_D3CR_VOSRDY_Msk)){;}

	stmcpp::reg::set(std::ref(RCC->APB4ENR), RCC_APB4ENR_SYSCFGEN);

	stmcpp::reg::set(std::ref(SYSCFG->PWRCR), SYSCFG_PWRCR_ODEN);
	tmpReg = stmcpp::reg::read(std::ref(SYSCFG->PWRCR), SYSCFG_PWRCR_ODEN_Msk);

	while(!stmcpp::reg::read(std::ref(PWR->D3CR), PWR_D3CR_VOSRDY_Msk)){;}
	

	//Set the HSE on and wait for it to be ready
	stmcpp::reg::set(std::ref(RCC->CR), RCC_CR_CSSHSEON);
	stmcpp::reg::set(std::ref(RCC->CR), RCC_CR_HSEON);
	while(!(stmcpp::reg::read(std::ref(RCC->CR)) & RCC_CR_HSERDY_Msk)){;}

	//Set the HSI48 on and wait for it to be ready
	stmcpp::reg::set(std::ref(RCC->CR), RCC_CR_HSI48ON);
	while(!(stmcpp::reg::read(std::ref(RCC->CR)) & RCC_CR_HSI48RDY_Msk)){;}

	//Set up the flash latency
	constexpr std::uint32_t FLASH_ACR_CFG = (FLASH_ACR_LATENCY_4WS | (0b11 << FLASH_ACR_WRHIGHFREQ_Pos));
	stmcpp::reg::write(std::ref(FLASH->ACR), FLASH_ACR_CFG);
	while(stmcpp::reg::read(std::ref(FLASH->ACR)) != FLASH_ACR_CFG){}

	//Setup the PLL (M=4 N=48 P=2 Q=16 R=2) -> sysclock = 384MHz

	stmcpp::clock::pll::setSource(stmcpp::clock::pll::clkSource::hse);
	stmcpp::clock::pll::pll<stmcpp::clock::pll::peripheral::pll1, 3, 60, 2, 4, 2> pll1(stmcpp::clock::pll::inputRange::f8_16MHz);
	pll1.enable();
	while(!pll1.isLocked()){;}

	//Setup the PLL for the USB
    /*stmcpp::clock::pll::pll<stmcpp::clock::pll::peripheral::pll3, 4, 15, 2, 3, 2> pll3(stmcpp::clock::pll::inputRange::f8_16MHz);
	pll3.enable();
	while(!pll3.isLocked()){;}*/

	//Setup the domain dividers
	stmcpp::clock::domain::domain domain(stmcpp::clock::domain::d1cpre::div1, stmcpp::clock::domain::d1ppre::div2, stmcpp::clock::domain::hpre::div2, stmcpp::clock::domain::d2ppre1::div2, stmcpp::clock::domain::d2ppre2::div2, stmcpp::clock::domain::d3ppre::div2, stmcpp::clock::domain::source::pll1);

	//Wait for the MCU to switch to PLL1 as the clock source
	while(domain.sourceStatus() != stmcpp::clock::domain::source::pll1){;}

	// Enable CSI (for clock compensation)
	stmcpp::reg::set(std::ref(RCC->CR), RCC_CR_CSION);
    while(!(stmcpp::reg::read(std::ref(RCC->CR)) & RCC_CR_CSION_Msk)){;}

	// Enable clock compensation cell and wait for it to be ready
	stmcpp::reg::set(std::ref(SYSCFG->CCCSR), SYSCFG_CCCSR_EN);
	while(!(stmcpp::reg::read(std::ref(SYSCFG->CCCSR)) & SYSCFG_CCCSR_READY_Msk)){;}

	// Select PLL3Q as clock for the usb peripheral
    stmcpp::reg::set(std::ref(RCC->D2CCIP2R), 0b11, RCC_D2CCIP2R_USBSEL_Pos);
	while(stmcpp::reg::read(std::ref(RCC->D2CCIP2R), 0b11, RCC_D2CCIP2R_USBSEL_Pos) != 0b11){;}
	
}