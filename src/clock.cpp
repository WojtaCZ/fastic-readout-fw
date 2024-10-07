#include "clock.hpp"
#include <stmcpp/register.hpp>
#include <stmcpp/units.hpp>
#include <stmcpp/clock.hpp>

using namespace stmcpp::units;

namespace clock {
	void init(){
		// We are running at 64MHz from HSI, setup systick to run at 1ms from that 
		stmcpp::clock::systick::enable(64_MHz, 1_ms);

		// Enable the LDO and check that the voltage levels are valid
		stmcpp::reg::set(std::ref(PWR->CR3), 1, PWR_CR3_LDOEN);
		stmcpp::reg::waitForBitSet(std::ref(PWR->CSR1), PWR_CSR1_ACTVOSRDY_Msk, []() { errorHandler.hardThrow(clock::error::ldo_timeout); });

		// Set voltage scale to 1 and wait for it to be ready
		stmcpp::reg::set(std::ref(PWR->D3CR), 0b11, PWR_D3CR_VOS_Pos);
		stmcpp::reg::waitForBitSet(std::ref(PWR->D3CR), PWR_D3CR_VOSRDY_Msk, []() { errorHandler.hardThrow(clock::error::vos1_timeout); });

		// Enable syscfg clock so that the overdrive can be enabled
		stmcpp::reg::set(std::ref(RCC->APB4ENR), RCC_APB4ENR_SYSCFGEN);
		
		// Enable overdrive (to VOS0) and wait for it to be ready
		stmcpp::reg::set(std::ref(SYSCFG->PWRCR), SYSCFG_PWRCR_ODEN);
		stmcpp::reg::waitForBitSet(std::ref(PWR->D3CR), PWR_D3CR_VOSRDY_Msk, []() { errorHandler.hardThrow(clock::error::vos0_timeout); });

		// Enable HSE and CSS and wait for it to be ready
		stmcpp::reg::set(std::ref(RCC->CR), RCC_CR_CSSHSEON | RCC_CR_HSEON);
		stmcpp::reg::waitForBitSet(std::ref(RCC->CR), RCC_CR_HSERDY_Msk, []() { errorHandler.hardThrow(clock::error::hse_timeout); });

		// Enable HSI48 and wait for it to be ready
		stmcpp::reg::set(std::ref(RCC->CR), RCC_CR_HSI48ON);
		stmcpp::reg::waitForBitSet(std::ref(RCC->CR), RCC_CR_HSI48RDY_Msk, []() { errorHandler.hardThrow(clock::error::hsi48_timeout); });

		// Enable CSI and wait for it to be ready
		stmcpp::reg::set(std::ref(RCC->CR), RCC_CR_CSION);
		stmcpp::reg::waitForBitSet(std::ref(RCC->CR), RCC_CR_CSIRDY_Msk, []() { errorHandler.hardThrow(clock::error::csi_timeout); });

		// Set up the flash latency to work with 480MHz
		constexpr std::uint32_t FLASH_ACR_CFG = (FLASH_ACR_LATENCY_4WS | (0b11 << FLASH_ACR_WRHIGHFREQ_Pos));
		stmcpp::reg::write(std::ref(FLASH->ACR), FLASH_ACR_CFG);
		stmcpp::reg::waitForBitsEqual(std::ref(FLASH->ACR), FLASH_ACR_LATENCY_Msk | FLASH_ACR_WRHIGHFREQ_Msk, FLASH_ACR_CFG, []() { errorHandler.hardThrow(clock::error::flash_latency_timeout); });

		// Setup the PLL1 (P = 480MHz, Q = 160MHz, R disabled)
		stmcpp::clock::pll::setSource(stmcpp::clock::pll::clkSource::hse);
		stmcpp::clock::pll::pll<stmcpp::clock::pll::peripheral::pll1, 3, 60, 2, 6, 0> pll1(stmcpp::clock::pll::inputRange::f8_16MHz);
		pll1.enable();
		duration timestamp_ = stmcpp::clock::systick::getDuration(); 
		while (!pll1.isLocked()) {
			if(stmcpp::clock::systick::getDuration() > (timestamp_ + 2000_ms)) errorHandler.hardThrow(clock::error::pll1_timeout);
		}

		//Setup the domain dividers and wait for the MCU to switch to PLL1 as the clock source
		stmcpp::clock::domain::domain domain(stmcpp::clock::domain::d1cpre::div1, stmcpp::clock::domain::d1ppre::div2, stmcpp::clock::domain::hpre::div2, stmcpp::clock::domain::d2ppre1::div2, stmcpp::clock::domain::d2ppre2::div2, stmcpp::clock::domain::d3ppre::div2, stmcpp::clock::domain::source::pll1);
		timestamp_ = stmcpp::clock::systick::getDuration(); 
		while (domain.sourceStatus() != stmcpp::clock::domain::source::pll1) {
			if(stmcpp::clock::systick::getDuration() > timestamp_ + 1000_ms) errorHandler.hardThrow(clock::error::pll1_timeout);
		}

		// We have switched to the new core clock, reconfigure systick aswell
		stmcpp::clock::systick::reconfigure(480_MHz, 1_ms);

		// Enable clock compensation cell and wait for it to be ready
		stmcpp::reg::set(std::ref(SYSCFG->CCCSR), SYSCFG_CCCSR_EN);
		stmcpp::reg::waitForBitSet(std::ref(SYSCFG->CCCSR), SYSCFG_CCCSR_READY_Msk, []() { errorHandler.hardThrow(clock::error::clock_compensation_timeout); });

		// Select HSI48 as clock for the usb peripheral
		static constexpr std::uint32_t usbClkSel = 0b11 << RCC_D2CCIP2R_USBSEL_Pos; 
		stmcpp::reg::set(std::ref(RCC->D2CCIP2R), usbClkSel);
		stmcpp::reg::waitForBitsEqual(std::ref(RCC->D2CCIP2R), RCC_D2CCIP2R_USBSEL_Msk, usbClkSel, []() { errorHandler.hardThrow(clock::error::clock_mux_timeout); });

		//Select CSI as clock for loww speed peripherals (I2C, UART)
		static constexpr std::uint32_t i2c123ClkSel = 0b01 << RCC_D2CCIP2R_I2C123SEL_Pos; 
		stmcpp::reg::set(std::ref(RCC->D2CCIP2R), i2c123ClkSel);
		stmcpp::reg::waitForBitsEqual(std::ref(RCC->D2CCIP2R), RCC_D2CCIP2R_I2C123SEL_Msk, i2c123ClkSel, []() { errorHandler.hardThrow(clock::error::clock_mux_timeout); });

		static constexpr std::uint32_t i2c4ClkSel = 0b01 << RCC_D3CCIPR_I2C4SEL_Pos; 
		stmcpp::reg::set(std::ref(RCC->D3CCIPR), i2c4ClkSel);
		stmcpp::reg::waitForBitsEqual(std::ref(RCC->D3CCIPR), RCC_D3CCIPR_I2C4SEL_Msk, i2c4ClkSel, []() { errorHandler.hardThrow(clock::error::clock_mux_timeout); });

		static constexpr std::uint32_t usart234578ClkSel = 0b100 << RCC_D2CCIP2R_USART28SEL_Pos; 
		stmcpp::reg::set(std::ref(RCC->D2CCIP2R), usart234578ClkSel);
		stmcpp::reg::waitForBitsEqual(std::ref(RCC->D2CCIP2R), RCC_D2CCIP2R_USART28SEL_Msk, usart234578ClkSel, []() { errorHandler.hardThrow(clock::error::clock_mux_timeout); });
		
	}
}