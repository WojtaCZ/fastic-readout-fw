#include "memory.hpp"
#include <stmcpp/register.hpp>

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