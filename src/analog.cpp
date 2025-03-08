#include "analog.hpp"
#include <stmcpp/adc.hpp>
#include <stmcpp/dma.hpp>
#include <stmcpp/bdma.hpp>
#include <stmcpp/dmamux.hpp>

namespace analog
{

    // Get the calibration values for the internal reference voltage
    uint32_t vrefIntCalibration = *(uint32_t*)(0x1FF1E860);
    // Get the temp calibration - resaling needs to be done because the values in TSCAL are measured with VREF = 3.3V
    double tscal1 = (double)(*(uint16_t*)(0x1ff1e820)) / (3.3 / 1.8);
    double tscal2 = (double)(*(uint16_t*)(0x1ff1e840)) / (3.3 / 1.8);

    // Set up ADC2 - 16 bit, circular DMA, 16x oversampling
    stmcpp::adc::adc<stmcpp::adc::peripheral::adc2> adc2 (stmcpp::adc::resolution::sixteenBit, stmcpp::adc::dataManegment::circularDMA, true, false, 16, 0, 4, true);
    // Set up the channels to be measured
    static constexpr stmcpp::adc::channel adc2_fastic2_vmon(14, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);
    static constexpr stmcpp::adc::channel adc2_dac1(16, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);
    static constexpr std::array adc2_sequence = {adc2_fastic2_vmon, adc2_dac1};
    // Create a buffer to store the measurements
    __attribute__((section(".dma_buffer"))) uint32_t adc2_measurements[adc2_sequence.size()];
    // Set up the DMA and DMAMUX for ADC2
    stmcpp::dmamux1::dmamux<stmcpp::dmamux1::channel::channel7> dmamux1ch7(stmcpp::dmamux1::request::adc2_dma);
    stmcpp::dma::dma<stmcpp::dma::peripheral::dma1, stmcpp::dma::stream::stream7> adc2_dma(stmcpp::dma::mode::periph2mem, stmcpp::dma::datasize::word, false, static_cast<uint32_t>(ADC2_BASE) + offsetof(ADC_TypeDef, DR), stmcpp::dma::datasize::word, true, (uint32_t)&adc2_measurements, 0, adc2_sequence.size(), stmcpp::dma::priority::low, true, stmcpp::dma::pincOffset::psize, false);

    // Set up ADC3 - 16 bit, circular DMA, 16x oversampling
    stmcpp::adc::adc<stmcpp::adc::peripheral::adc3> adc3 (stmcpp::adc::resolution::sixteenBit, stmcpp::adc::dataManegment::circularDMA, true, false, 16, 0, 4, true);
    // Set up the channels to be measured
    static constexpr stmcpp::adc::channel adc3_fastic1_vmon(13, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);
    static constexpr stmcpp::adc::channel adc3_vsense(18, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);
    static constexpr stmcpp::adc::channel adc3_vrefint(19, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);
    static constexpr stmcpp::adc::channel adc3_vbat(17, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);
    static constexpr std::array adc3_sequence = {adc3_fastic1_vmon, adc3_vsense, adc3_vrefint, adc3_vbat};
    // Create a buffer to store the measurements
    __attribute__((section(".bdma_buffer")))  uint32_t adc3_measurements[adc3_sequence.size()];
    // Set up the DMA and DMAMUX for ADC3
    stmcpp::dmamux2::dmamux<stmcpp::dmamux2::channel::channel0> dmamux2ch0(stmcpp::dmamux2::request::adc3_dma);
    stmcpp::bdma::bdma<stmcpp::bdma::peripheral::bdma, stmcpp::bdma::channel::channel0> adc3_dma(stmcpp::bdma::mode::periph2mem, stmcpp::bdma::dataSize::word, false, static_cast<uint32_t>(ADC3_BASE) + offsetof(ADC_TypeDef, DR), stmcpp::bdma::dataSize::word, true, (uint32_t)&adc3_measurements, 0, adc3_sequence.size(), stmcpp::bdma::priority::low, true, stmcpp::bdma::pincOffset::psize, false);

    double voltageMultiplier = 0;

    void init(){
        // Set up timer 15 used to trigger the ADCs to generate an event at 100Hz
        stmcpp::reg::write(std::ref(TIM15->PSC), 24-1);   
        stmcpp::reg::write(std::ref(TIM15->ARR), 10000-1);
        stmcpp::reg::write(std::ref(TIM15->CR2), 0b010<< TIM_CR2_MMS_Pos);

        // Set up the reference buffer - 1.8V output
        stmcpp::reg::write(std::ref(VREFBUF->CSR), VREFBUF_CSR_ENVR | VREFBUF_CSR_VRS_OUT3);  
        stmcpp::reg::waitForBitSet(std::ref(VREFBUF->CSR), VREFBUF_CSR_VRR_Msk);

        // Enable the auxilary measurement channels of ADC3 (internal vref, temperature sensor and battery voltage)
        stmcpp::reg::set(std::ref(ADC3_COMMON->CCR), ADC_CCR_VREFEN | ADC_CCR_TSEN | ADC_CCR_VBATEN);

        // Calibrate the ADCs
        adc2.calibrate(stmcpp::adc::calibration::singleEnded, true);
        adc3.calibrate(stmcpp::adc::calibration::singleEnded, true);

        // Set up the ADC sequences with the propper trigger - timer 15
        adc2.setupRegularSequence(adc2_sequence, 0b01110, stmcpp::adc::hardwareTrigEdge::rising);
        adc3.setupRegularSequence(adc3_sequence, 0b01110, stmcpp::adc::hardwareTrigEdge::rising);

        // Set up the DMA
        adc2_dma.setNumberOfData(adc2_sequence.size());
        adc3_dma.setNumberOfData(adc3_sequence.size());
        adc2_dma.enable();
        adc3_dma.enable();

        // Enable ADC3 - we first need to measure the internal reference to calibrate the VREFBUF
        adc3.enable();
        adc3.startRegular();

        // Enable the timer and its output
        //stmcpp::reg::set(std::ref(TIM15->CR1), TIM_CR1_CEN);

        // Wait for the ADC3 to finish the measurement
        //stmcpp::reg::waitForBitSet(std::ref(BDMA->ISR), BDMA_ISR_TCIF0);

        // Disable the timer
        //stmcpp::reg::clear(std::ref(TIM15->CR1), TIM_CR1_CEN);

        // Calculate the voltage multiplier based on the VREFBUF and VREFINT measurements 
        //calibrateVoltageMultiplier();

        // Enable the second ADC
        adc2.enable();
        adc2.startRegular();

        // Enable transfer complete interrupts
        adc2_dma.enableInterrupt(stmcpp::dma::interrupt::transferComplete);
        adc3_dma.enableInterrupt(stmcpp::bdma::interrupt::transferComplete);
        NVIC_EnableIRQ(DMA1_Stream7_IRQn);
        NVIC_EnableIRQ(BDMA_Channel0_IRQn);

        // Start the timer to begin sampling
        stmcpp::reg::set(std::ref(TIM15->CR1), TIM_CR1_CEN);

    }

    void calibrateVoltageMultiplier(){
        voltageMultiplier = (((double)vrefIntCalibration * (3.3 / 65535)) / (double)adc3_measurements[2]);
    }

    double getVoltageMultiplier(){
        return voltageMultiplier;
    }

    double getFastIC2Voltage(){
        return adc2_measurements[0] * voltageMultiplier;
    }

    double getFastIC1Voltage(){
        return adc3_measurements[0] * voltageMultiplier;
    }

    double getVbatVoltage(){
        return adc3_measurements[3] * voltageMultiplier * 4.0;
    }

    double getTemperature(){
        return ((110.0 - 30.0) / (double)(tscal2 - tscal1)) * (double)(adc3_measurements[1] - tscal1) + 30.0;
    }
} 


extern "C" void BDMA_CH0_IRQHandler(void)
{
    __ASM volatile("bkpt");
    NVIC_ClearPendingIRQ(BDMA_Channel0_IRQn);
    analog::adc3_dma.clearInterruptFlag(stmcpp::bdma::interrupt::transferComplete);
}


extern "C" void DMA1_STR7_IRQHandler(){
    __ASM volatile("bkpt");
    NVIC_ClearPendingIRQ(DMA1_Stream7_IRQn);
    analog::adc2_dma.clearInterruptFlag(stmcpp::dma::interrupt::transferComplete);

}

extern "C" void TIM15_IRQHandler() {

    NVIC_ClearPendingIRQ(TIM15_IRQn);
}