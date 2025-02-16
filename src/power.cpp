#include "power.hpp"
#include <stmcpp/register.hpp>
#include <stmcpp/gpio.hpp>
#include <stmcpp/dac.hpp>
#include <stmcpp/adc.hpp>
#include <stmcpp/dma.hpp>
#include <stmcpp/dmamux.hpp>
#include <array>


uint32_t vrefIntCalibration = *(uint32_t*)(0x1FF1E860);

namespace power { 

    // 1V2 power control pins
    stmcpp::gpio::pin<stmcpp::gpio::port::portk, 1> pg_1V2D(stmcpp::gpio::mode::input);
    stmcpp::gpio::pin<stmcpp::gpio::port::portk, 2> pg_1V2T(stmcpp::gpio::mode::input);
    stmcpp::gpio::pin<stmcpp::gpio::port::portg, 2> pg_1V2A(stmcpp::gpio::mode::input);
    
    stmcpp::gpio::pin<stmcpp::gpio::port::portk, 5> en_1V2(stmcpp::gpio::mode::output);

    // 1V8 power control pin
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 9> pg_1V8D(stmcpp::gpio::mode::input);

    // 3V3A power control pin
    stmcpp::gpio::pin<stmcpp::gpio::port::portg, 10> pg_3V3A(stmcpp::gpio::mode::input);


    bool isPowerGood(ldo regulator) {
        switch(regulator) {
            case ldo::D1V2:
                return pg_1V2D.read();
                break;
            case ldo::T1V2:
                return pg_1V2T.read();
                break;
            case ldo::A1V2:
                return pg_1V2A.read();
                break;
            case ldo::D1V8:
                return pg_1V8D.read();
                break;
            case ldo::A3V3:
                return pg_3V3A.read();
                break;

            default:
                return false;
                break;
        }
    }

    void enable1V2() { 
        en_1V2.set();
    }

    void disable1V2() {
        en_1V2.clear();
    }
} 

namespace hv {

    
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 5> shutdown_n(stmcpp::gpio::mode::output);
    stmcpp::gpio::pin<stmcpp::gpio::port::porta, 4> control(stmcpp::gpio::mode::analog);
    stmcpp::gpio::pin<stmcpp::gpio::port::portf, 11> isense(stmcpp::gpio::mode::analog);
    stmcpp::gpio::pin<stmcpp::gpio::port::portf, 12> usense(stmcpp::gpio::mode::analog);

    stmcpp::adc::adc<stmcpp::adc::peripheral::adc1> adc1 (stmcpp::adc::resolution::sixteenBit, stmcpp::adc::dataManegment::oneShotDMA, true, false, 16, 0, 4, true);

    static constexpr stmcpp::adc::channel adc1_inp2(2, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);
    static constexpr stmcpp::adc::channel adc1_inp6(6, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);

    static constexpr std::array adc1_sequence = {adc1_inp2, adc1_inp6};

    static constexpr auto adcMeasurementsSize = adc1_sequence.size();
    __attribute__((section(".dma_buffer"))) uint32_t adcMeasurements[adcMeasurementsSize];

    stmcpp::dac::dac<stmcpp::dac::channel::ch1> dac1_ch1;

    stmcpp::dmamux1::dmamux<stmcpp::dmamux1::channel::channel2> dmamux1ch2(stmcpp::dmamux1::request::adc1_dma);
    stmcpp::dma::dma<stmcpp::dma::peripheral::dma1, stmcpp::dma::stream::stream2> adc_dma(stmcpp::dma::mode::periph2mem, stmcpp::dma::datasize::word, false, static_cast<uint32_t>(ADC1_BASE) + offsetof(ADC_TypeDef, DR), stmcpp::dma::datasize::word, true, (uint32_t)&adcMeasurements[0], 0, adcMeasurementsSize, stmcpp::dma::priority::low, false, stmcpp::dma::pincOffset::psize, false);



    stmcpp::adc::adc<stmcpp::adc::peripheral::adc3> adc3 (stmcpp::adc::resolution::sixteenBit, stmcpp::adc::dataManegment::storeInDR, true, false, 16, 0, 4, true);

    static constexpr stmcpp::adc::channel vsense(18, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);
    static constexpr stmcpp::adc::channel vrefint(19, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);
    static constexpr stmcpp::adc::channel vbat(17, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);

    static constexpr std::array adc3_sequence = {vrefint};

    uint32_t vrefIntMeasurement;

    double referenceVoltage = 1.8;



    void init(){

        // Timer 15 is used as a trigger (generate a 100kHz clock) 
        stmcpp::reg::write(std::ref(TIM12->PSC), 24-1);   
        stmcpp::reg::write(std::ref(TIM12->ARR), 10000);
        // Load all the registers
        stmcpp::reg::set(std::ref(TIM12->EGR), TIM_EGR_UG);

        // Enable Timer 12 interrupt
        stmcpp::reg::set(std::ref(TIM12->DIER), TIM_DIER_UIE);
        //NVIC_EnableIRQ(TIM8_BRK_TIM12_IRQn);

        //Set up the reference buffer
        stmcpp::reg::write(std::ref(VREFBUF->CSR), VREFBUF_CSR_ENVR | VREFBUF_CSR_VRS_OUT3);  
        stmcpp::reg::waitForBitSet(std::ref(VREFBUF->CSR), VREFBUF_CSR_VRR_Msk);

        stmcpp::reg::set(std::ref(ADC3_COMMON->CCR), ADC_CCR_VREFEN | ADC_CCR_TSEN | ADC_CCR_VBATEN);
        // Set up the ADC
        adc3.calibrate(stmcpp::adc::calibration::singleEnded, true);

        adc3.setupRegularSequence(adc3_sequence);
        adc3.enable();
        adc3.startRegular();
        stmcpp::reg::waitForBitSet(std::ref(ADC3->ISR), ADC_ISR_EOS);
        vrefIntMeasurement = ADC3->DR;
        adc3.disable();

        referenceVoltage = (((double)vrefIntCalibration * (3.3/65535)) / vrefIntMeasurement);
    





        // Set up the ADC
        stmcpp::reg::set(std::ref(ADC12_COMMON->CCR), 0, ADC_CCR_PRESC_Pos);
        adc1.calibrate(stmcpp::adc::calibration::singleEnded, true);
        adc1.setupRegularSequence(adc1_sequence);

        adc_dma.enableInterrupt(stmcpp::dma::interrupt::transferComplete);
        NVIC_EnableIRQ(DMA1_Stream2_IRQn);
        adc_dma.setNumberOfData(adcMeasurementsSize);
        adc_dma.disableDoubleBuffer();
        /*adc1.enableInterrupt(stmcpp::adc::interrupt::endOfConversion);
        adc1.enableInterrupt(stmcpp::adc::interrupt::endOfSequence);
        adc1.clearInterruptFlag(stmcpp::adc::interrupt::endOfConversion);
        adc1.clearInterruptFlag(stmcpp::adc::interrupt::endOfSequence);*/

        //NVIC_EnableIRQ(ADC_IRQn);
        dac1_ch1.enable();
        dac1_ch1.setValue(4095);

        shutdown_n.set();
        //dac1_ch1.trigger();

        adc1.enable();
        adc_dma.enable();


        hv::adc1.startRegular();

        //Enable the timer and its output
        //stmcpp::reg::set(std::ref(TIM12->CR1), TIM_CR1_CEN);
    } 
      
}


extern "C" void DMA_STR2_IRQHandler(){
    __ASM volatile("bkpt");
    hv::adc_dma.clearInterruptFlag(stmcpp::dma::interrupt::transferComplete);
    NVIC_ClearPendingIRQ(DMA1_Stream2_IRQn);
    hv::adc_dma.enable();
    hv::adc1.startRegular();

}

extern "C" void ADC1_2_IRQHandler(){
	

    if(stmcpp::reg::read(std::ref(ADC1->ISR), ADC_ISR_EOS)) {
        //hv::measurementIdx = 0;
        
        hv::adc1.clearInterruptFlag(stmcpp::adc::interrupt::endOfSequence);
    }else if(stmcpp::reg::read(std::ref(ADC1->ISR), ADC_ISR_EOC)){
        stmcpp::reg::waitForBitSet(std::ref(ADC1->ISR), ADC_ISR_EOC);
        //hv::measurements[hv::measurementIdx++] = ADC1->DR;
        hv::adc1.clearInterruptFlag(stmcpp::adc::interrupt::endOfConversion);
        //hv::adc1.startRegular();
    }else{
        __ASM volatile("bkpt");
    }


    NVIC_ClearPendingIRQ(ADC_IRQn);
}



extern "C" void TIM8_BRK_TIM12_IRQHandler() {
    if (stmcpp::reg::read(std::ref(TIM12->SR), TIM_SR_UIF_Msk)) {
        // Clear the update interrupt flag
        stmcpp::reg::clear(std::ref(TIM12->SR), TIM_SR_UIF_Msk);
       // hv::measurementIdx = 0;
        hv::adc1.startRegular();
    }

    NVIC_ClearPendingIRQ(TIM8_BRK_TIM12_IRQn);
}