#include "stm32h743xx.h"
#include "clock.hpp"

namespace clock{

    //Initialize the clock -> SYSCLK = 320MHz, timer clock = 160MHz
    std::uint8_t init(){

        //Max number of cycles to be waited for the HSI to lock (1 cycle ~ 15.6ns)
        static constexpr int HSI_LOCK_MAXWAIT = 64102; // approx 1ms
        //Max number of cycles to be waited for the PLL to lock (1 cycle ~ 62.5ns)
        static constexpr int PLL_LOCK_MAXWAIT = 16000; // approx 1ms
        //Max number of cycles to be waited for the FLASH latency to be set (1 cycle ~ 62.5ns)
        static constexpr int LATENCY_SET_MAXWAIT = 16000; // approx 1ms
        //Max number of cycles to be waited for the MCU to switch from HSI to the PLL (1 cycle ~ 62.5ns)
        static constexpr int SWS_LOCK_MAXWAIT = 16000; // approx 1ms

        //Set the HSI divider to 4 -> HSI_CLKOUT = 16MHz
        RCC->CR |= RCC_CR_HSIDIV_4;

        int waitcycles = 0;

        //Wait for the HSI to be ready and divider to be set
        while(!(RCC->CR & RCC_CR_HSIRDY_Msk) | !(RCC->CR & RCC_CR_HSIDIVF_Msk)){
            //If the wait time was longer than expected
            if(waitcycles >= HSI_LOCK_MAXWAIT){
                //Shut down the HSI
                RCC->CR &= ~(1 << RCC_CR_HSION_Pos);
                //Signal an error
                return 1;
            }
            waitcycles++;
        }

        //Set HPRE and D1PRE to divide by 2
        RCC->D1CFGR |= RCC_D1CFGR_HPRE_DIV2 | RCC_D1CFGR_D1PPRE_DIV2;
        //Set D2PRE1 and D2PRE2 to divide by 2
        RCC->D2CFGR |= RCC_D2CFGR_D2PPRE1_DIV2 | RCC_D2CFGR_D2PPRE2_DIV2;
        //Set D3PRE to divide by 2
        RCC->D3CFGR |= RCC_D3CFGR_D3PPRE_DIV2;

        //Enable P1 divider output and set the PLL input range to be 8-16MHz
        RCC->PLLCFGR |= RCC_PLLCFGR_DIVP1EN | RCC_PLLCFGR_PLL1RGE_3;
        //Set the M1 clock divisor to 1
        RCC->PLLCKSELR = (1 << RCC_PLLCKSELR_DIVM1_Pos);
        //Set up PLL divider P = 2, N = 40, Q = 2, R = 2
        RCC->PLL1DIVR = (1 << RCC_PLL1DIVR_P1_Pos) | (39 << RCC_PLL1DIVR_N1_Pos) | (1 << RCC_PLL1DIVR_Q1_Pos) | (1 << RCC_PLL1DIVR_R1_Pos);

        //Enable the PLL
        RCC->CR |= (1 << RCC_CR_PLL1ON_Pos);

        waitcycles = 0;

        //Wait for the PLL to be ready
        while(!(RCC->CR & RCC_CR_PLLRDY_Msk)){
            //If the wait time was longer than expected
            if(waitcycles >= PLL_LOCK_MAXWAIT){
                //Shut down the PLL
                RCC->CR &= ~(1 << RCC_CR_PLL1ON_Pos);
                //Signal an error
                return 1;
            }
            waitcycles++;
        }

        //Set up the flash latency
        constexpr std::uint32_t FLASH_ACR_CFG = (FLASH_ACR_LATENCY_2WS | (0b01 << FLASH_ACR_WRHIGHFREQ_Pos));
        FLASH->ACR = FLASH_ACR_CFG;

        waitcycles = 0;

        //Wait for the FLASH to apply the changes
        while((FLASH->ACR != FLASH_ACR_CFG)){
            //If the wait time was longer than expected
            if(waitcycles >= LATENCY_SET_MAXWAIT){
                //Change the FLASH latency back to default
                FLASH->ACR = 37;
                //Signal an error
                return 1;
            }
            waitcycles++;
        }

        //Set the system clock source to PLL1
        RCC->CFGR |= RCC_CFGR_SW_PLL1;

        waitcycles = 0;

        //Wait for the MCU to switch to PLL1 as the clock source
        while((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL1){
            //If the wait time was longer than expected
            if(waitcycles >= SWS_LOCK_MAXWAIT){
                //Change the switch to the HSI
                RCC->CFGR &= ~RCC_CFGR_SW_HSI;
                //Signal an error
                return 1;
            }
            waitcycles++;
        }

        //If all went well
        return 0;

    }

    
}
