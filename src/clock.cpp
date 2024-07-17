#include "clock.hpp"
#include <stmcpp/register.hpp>
#include <stmcpp/clock.hpp>

void clock::init(){
    //Set voltage scale to 1
	stmcpp::reg::set(std::ref(PWR->D3CR), 0b11, PWR_D3CR_VOS_Pos);

	//reg::set(std::ref(RCC->APB4ENR), RCC_APB4ENR_SYSCFGEN);

	//Enable the core voltage overdrive and wait for it to be set
	//reg::set(std::ref(SYSCFG->PWRCR), SYSCFG_PWRCR_ODEN);
	//while(!reg::read(std::ref(PWR->D3CR), 1, PWR_D3CR_VOSRDY_Pos)){;}

	//Set the HSE on and wait for it to be ready
	stmcpp::reg::set(std::ref(RCC->CR), RCC_CR_CSSHSEON);
	stmcpp::reg::set(std::ref(RCC->CR), RCC_CR_HSEON);
	while(!(stmcpp::reg::read(std::ref(RCC->CR)) & RCC_CR_HSERDY_Msk)){;}

	//Set up the flash latency
	constexpr std::uint32_t FLASH_ACR_CFG = (FLASH_ACR_LATENCY_4WS | (0b11 << FLASH_ACR_WRHIGHFREQ_Pos));
	stmcpp::reg::write(std::ref(FLASH->ACR), FLASH_ACR_CFG);
	while(stmcpp::reg::read(std::ref(FLASH->ACR)) != FLASH_ACR_CFG){}

	//Setup the PLL (M=3 N=50 P=2 Q=16 R=2) -> sysclock = 400MHz
	stmcpp::clock::pll::setSource(stmcpp::clock::pll::clkSource::hse);
	stmcpp::clock::pll::pll<stmcpp::clock::pll::peripheral::pll1, 3, 50, 2, 16, 2> pll1(stmcpp::clock::pll::inputRange::f8_16MHz);
	pll1.enable();
	while(!pll1.isLocked()){;}

	//Setup the domain dividers
	stmcpp::clock::domain::domain domain(stmcpp::clock::domain::d1cpre::div1, stmcpp::clock::domain::d1ppre::div2, stmcpp::clock::domain::hpre::div2, stmcpp::clock::domain::d2ppre1::div2, stmcpp::clock::domain::d2ppre2::div2, stmcpp::clock::domain::d3ppre::div2, stmcpp::clock::domain::source::pll1);

	//Wait for the MCU to switch to PLL1 as the clock source
	while(domain.sourceStatus() != stmcpp::clock::domain::source::pll1){;}
}