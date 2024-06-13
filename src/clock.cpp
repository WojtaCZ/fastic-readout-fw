#include "stm32h753xx.h"
#include "clock.hpp"

namespace clock{

    void init(){
        //Set the HSI divider to 4 -> HSI_CLKOUT = 16MHz
        reg::set(std::ref(RCC->CR), RCC_CR_HSIDIV_4);

        //Wait for the HSI to be ready and divider to be set
        while(!(reg::read(std::ref(RCC->CR)) & RCC_CR_HSIRDY_Msk) | !(reg::read(std::ref(RCC->CR)) & RCC_CR_HSIDIVF_Msk)){;}

        //Setup the domain dividers
        domain::domain domain(domain::d1cpre::div1, domain::d1ppre::div2, domain::hpre::div2, domain::d2ppre1::div2, domain::d2ppre2::div2, domain::d3ppre::div2, domain::source::pll1);

        //Setup the PLL
        pll::pll<pll::peripheral::pll1, 1, 40, 2, 4, 2> pll1(pll::inputrange::f8_16MHz);
        pll1.enable();
        while(!pll1.isLocked()){;}

        //Enable the CSI (for compensation cell)
        reg::set(std::ref(RCC->CR), RCC_CR_CSION);

        //Wait for the CSI to become ready
        while(!(reg::read(std::ref(RCC->CR)) & RCC_CR_CSION_Msk)){;};


        //Set up the flash latency
        constexpr std::uint32_t FLASH_ACR_CFG = (FLASH_ACR_LATENCY_2WS | (0b01 << FLASH_ACR_WRHIGHFREQ_Pos));
        reg::write(std::ref(FLASH->ACR), FLASH_ACR_CFG);
        while(reg::read(std::ref(FLASH->ACR)) != FLASH_ACR_CFG){}

        //Set the system clock source to PLL1
        reg::set(std::ref(RCC->CFGR), RCC_CFGR_SW_PLL1);
    
        //Wait for the MCU to switch to PLL1 as the clock source
        while((reg::read(std::ref(RCC->CFGR)) & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL1){}
    }

    
}
