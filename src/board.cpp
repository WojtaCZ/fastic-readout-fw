#include "board.hpp"

#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

#include <stmcpp/register.hpp>
#include <stmcpp/gpio.hpp>

namespace memory{
    void disableCachingD2(){
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

        __ASM volatile("dsb");
        __ASM volatile("isb");
    
    }

    void disableCachingD3(){
        // Configure the MPU to not cache D2 RAM - this is needed for the DMA buffers
        // We are configuring region 0
        stmcpp::reg::write(std::ref(MPU->RNR), 1);
        
        // Make sure the region is disabled
        stmcpp::reg::clear(std::ref(MPU->RASR), MPU_RASR_ENABLE_Msk);

        // Set the region base address - RAM D2
        stmcpp::reg::write(std::ref(MPU->RBAR), 0x38000000); 

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

        __ASM volatile("dsb");
        __ASM volatile("isb");
    
    }

    void enableMPU(){
        // Enable the MPU
        stmcpp::reg::write(std::ref(MPU->CTRL), 
            (0b1 << MPU_CTRL_PRIVDEFENA_Pos) |
            (0b0 << MPU_CTRL_HFNMIENA_Pos) |
            (0b1 << MPU_CTRL_ENABLE_Pos) 
        );
    
        __ASM volatile("dsb");
        __ASM volatile("isb");
    }
}

namespace rng{
    void init(){
        // Enable the generator
        stmcpp::reg::set(std::ref(RNG->CR), RNG_CR_RNGEN);
    }

    std::uint32_t get(){
        // Wait for the data to be ready
        while(!stmcpp::reg::read(std::ref(RNG->SR), RNG_SR_DRDY_Msk));

        return stmcpp::reg::read(std::ref(RNG->DR));
    }
}

namespace board {
    stmcpp::gpio::pin<stmcpp::gpio::port::porti, 5> ledRed(stmcpp::gpio::mode::output);
    stmcpp::gpio::pin<stmcpp::gpio::port::porti, 6> ledGreen(stmcpp::gpio::mode::output);
    stmcpp::gpio::pin<stmcpp::gpio::port::porti, 7> ledBlue(stmcpp::gpio::mode::output);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 10> ledUSB(stmcpp::gpio::mode::output);

    status status_ = status::OK;
    uint8_t processCount = 0;

    status getStatus() {
        return status_;
    }

    void setStatus(status s) {
        status_ = s;
        processStatus();
    }

    void processStatus() {
        if(status_ == status::OK) {
            if(processCount % 4 == 0) {
                ledRed.clear();
                ledGreen.set();
                ledBlue.clear();
            } else {
                ledRed.clear();
                ledGreen.clear();
                ledBlue.clear();
            }
        } else if (status_ == status::STREAMING) {
                ledRed.clear();
                ledGreen.clear();
                ledBlue.clear();
        } else if (status_ == status::ERROR) {
            ledRed.toggle();
        }

        processCount++;

        if(processCount == 4) {
            processCount = 0;
        }
    }
}